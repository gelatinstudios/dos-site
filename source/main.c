
#include "main.h"

WAJIC(void,  go_to_page, (const char *url, int length), 
      {
          window.open(MStrGet(url, length));
      })

void put_string(int x, int y, char *text) {
    const unsigned char bg = 0;
    const unsigned char fg = 1;
    
    int w = screenwidth();
    
    unsigned char *screen = screenbuffer();
    
    unsigned char *p = screen + 2*(x + y*w);
    for (char *c = text; *c; c++) {
        *p++ = *c;
        *p++ = bg << 4 | fg;
    }
}

void put_string_centered(int y, char *text) {
    put_string(screenwidth()/2 - strlen(text)/2, y, text);
}

// returns height of the text
int put_cell_text(State *state, int start_x, int start_y, int max_width, char *text) {
    Link *selected_link = get_selected_link(state);
    
    u16 *screen = (u16 *)screenbuffer();
    int w = screenwidth();
    
    Tokenizer tokenizer = {text};
    int x = start_x;
    int y = start_y;
    Token token;
    while (get_token(&tokenizer, &token)) {
        if (x + token.length - start_x >= max_width) {
            y++;
            x = start_x;
        }
        
        u16 fg = 1;
        u16 bg = 0;
        if (token.url) {
            fg = 2;
            bg = 0;
        }
        if (selected_link->text_start == token.s) {
            fg = 0;
            bg = 1;
        }
        
        for (int i = 0; i < token.length; i++) {
            char c = token.s[i];
            screen[x + y*w] = (bg << 12) | (fg << 8) | c;
            x++;
        }
        x++;
    }
    
    return y - start_y + 1;
}

int get_cell_text_height(int max_width, char *text) {
    int w = screenwidth();
    
    int x = 0;
    int result = 1;
    
    Tokenizer tokenizer = {text};
    Token token;
    while (get_token(&tokenizer, &token)) {
        if (x + token.length  >= max_width) {
            result++;
            x = 0;
        }
        
        x += token.length + 1;
    }
    
    return result;
}

int move_selected_link(State *state, int drow, int dcol) {
    int dindex = 0;
    if (drow > 0 || dcol > 0) {
        dindex = 1;
    } else {
        dindex = -1;
    }
    
    int new_index = state->index + dindex;
    if (new_index >= 0 && new_index < get_selected_cell(state)->link_count) {
        state->index = new_index;
        return true;
    }
    
    int n = 1;
    for (;;) {
        int row = state->row + drow*n;
        int col = state->col + dcol*n;
        n++;
        
        if (row < 0 || row >= ArrayCount(table))    return false;
        if (col < 0 || col >= ArrayCount(table[0])) return false;
        
        Cell_Links *links = &state->cell_links[row][col];
        if (links->link_count) {
            state->row = row;
            state->col = col;
            if (dindex > 0) {
                state->index = 0;
            } else {
                state->index = links->link_count-1;
            }
            return true;
        }
    }
    
    return false;
}

// dos-like apparently uses 0..=63 so i'm coping
void set_pal(int index, int r, int g, int b) {
    r = (int)(((float)r / 255.0) * 63);
    g = (int)(((float)g / 255.0) * 63);
    b = (int)(((float)b / 255.0) * 63);
    setpal(index, r, g, b);
}

struct sound_t *generate_sound_effect(int freq) {
    HexWave osc;
    hexwave_create(&osc, 1, 0, 0, 0);
    
    int sample_rate = 44100;
    
    int sample_count = sample_rate / 16; // half a second
    
    float *pulse_data_raw = (float *)malloc(sample_count*sizeof(float));
    
    hexwave_generate_samples(pulse_data_raw, sample_count, &osc, (float)freq / (float)sample_rate);
    
    short *pulse_data = (short *)malloc(sample_count*sizeof(u16));
    for (int i = 0; i < sample_count; i++) {
        pulse_data[i] = (short)(32767.0*pulse_data_raw[i]);
    }
    
    struct sound_t *sound = createsound(1, sample_rate, sample_count, pulse_data);
    
    free(pulse_data_raw);
    free(pulse_data);
    
    return sound;
}

int main(int argc, char **argv) {
    State state = {0};
    
    int first_link_found = 0;
    for (int row = 0; row < ArrayCount(table); row++) {
        for (int col = 0; col < ArrayCount(table[row]); col++) {
            char *cell = table[row][col];
            
            Tokenizer tokenizer = {cell};
            Token token;
            while (get_token(&tokenizer, &token)) {
                if (token.url) {
                    Cell_Links *l = &state.cell_links[row][col];
                    Link new_link = {token.s, token.url, token.url_length};
                    if (!first_link_found)  {
                        state.row = row;
                        state.col = col;
                        state.index = l->link_count;
                        first_link_found = 1;
                    }
                    l->links[l->link_count++] = new_link;
                }
            }
        }
    }
    
    hexwave_init(32, 16, 0);
    
    struct sound_t *moved_sound = generate_sound_effect(110);
    struct sound_t *selected_sound = generate_sound_effect(220);
    
    setvideomode( videomode_80x43_8x8);
    cursoff();
    set_pal(0, 68, 56, 76);
    set_pal(1, 205, 203, 216);
    set_pal(2, 134, 249, 215);
    
    char keys_down_last_frame[KEYCOUNT] = {0};
    
    while (!shuttingdown()) {
        waitvbl();
        
        enum keycode_t keys_pressed[KEYCOUNT] = {0};
        
        for (int i =0; i < KEYCOUNT; i++) {
            if (!keys_down_last_frame[i] && keystate(i)) {
                keys_pressed[i] = 1;
            }
        }
        
        if (keys_pressed[KEY_RETURN]) {
            playsound(0, selected_sound, 0, 128);
        }
        
        // we wait until the key is unpressed to actually load the page
        // otherwise, when the user tabs back in, the key is still considered pressed
        if (keys_down_last_frame[KEY_RETURN] && !keystate(KEY_RETURN)) {
            Link *selected_link = get_selected_link(&state);
            go_to_page(selected_link->url, selected_link->url_length);
        }
        
        int moved = 0;
        if (keys_pressed[KEY_LEFT])  moved |= move_selected_link(&state,  0, -1);
        if (keys_pressed[KEY_RIGHT]) moved |= move_selected_link(&state,  0,  1);
        if (keys_pressed[KEY_UP])    moved |= move_selected_link(&state, -1,  0);
        if (keys_pressed[KEY_DOWN])  moved |= move_selected_link(&state,  1,  0);
        
        if (moved) {
            playsound(0, moved_sound, 0, 128);
        }
        
        int y = 2;
        put_string_centered(y++, "Welcome to Gelatin Studios!");
        put_string_centered(y++, "I am not an actual dev studio! I'm just one person!");
        put_string_centered(y++, "I'm currently employed full-time as a software engineer.");
        put_string_centered(y++, "In my spare time, I still like to work on my own projects.");
        put_string_centered(y++, "Here's a showcase of some of them that I've put on the web!");
        put_string_centered(y++, "All of these pages (including this one!) can be downloaded and used offline!");
        put_string_centered(y++, "There will be more to come soon!");
        y += 3;
        
        int cell_width = (screenwidth()-4)/3 - 1;
        
        int full_table_height = 0;
        for (int row = 0; row < ArrayCount(table); row++) {
            int max_height = 0;
            for (int col = 0; col < ArrayCount(table[row]); col++) {
                int height = get_cell_text_height(cell_width, table[row][col]) + 1;
                if (height > max_height) max_height = height;
            }
            full_table_height += max_height + 1;
        }
        
        int padding = ((screenheight() - y) - full_table_height) / ArrayCount(table);
        
        int x = 1;
        for (int row = 0; row < ArrayCount(table); row++) {
            int max_height = 0;
            for (int col = 0; col < ArrayCount(table[row]); col++) {
                int height = put_cell_text(&state, 2 + cell_width*col + col, y, cell_width, table[row][col]);
                if (height > max_height) max_height = height;
            }
            y += max_height + 1 + padding;
        }
        
        for (int i = 0; i < KEYCOUNT; i++) {
            keys_down_last_frame[i] = keystate(i);
        }
    }
    
    return 0;
}