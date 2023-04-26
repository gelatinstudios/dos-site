#ifndef MAIN_H
#define MAIN_H

#include <string.h>
#include <stdint.h>

#include "../wasm/wajic.h"
#define DOS_IMPLEMENTATION
#include "../vendor/dos.h"

#define STB_HEXWAVE_IMPLEMENTATION
#include "../vendor/stb_hexwave.h"

#define ArrayCount(arr) (sizeof(arr)/sizeof((arr)[0]))

typedef uint16_t u16;

typedef struct {
    char *s;
    int length;
    
    char *url;
    int url_length;
} Token;

typedef struct {
    char *s;
    int at;
} Tokenizer;

int get_token(Tokenizer *tokenizer, Token *token) {
    while (isspace(tokenizer->s[tokenizer->at]))
        tokenizer->at++;
    
    if (!tokenizer->s[tokenizer->at]) return false;
    
    token->s = &tokenizer->s[tokenizer->at];
    token->length = 0;
    token->url = 0;
    token->url_length = 0;
    int is_link = false;
    if (tokenizer->s[tokenizer->at] == '<') {
        tokenizer->at++;
        token->s++;
        is_link = true;
    }
    
    while (tokenizer->s[tokenizer->at]) {
        if (!is_link && isspace(tokenizer->s[tokenizer->at])) 
            break;
        
        if (is_link && tokenizer->s[tokenizer->at] == '|')
            break;
        
        tokenizer->at++;
        token->length++;
    }
    
    if (is_link) {
        tokenizer->at++; // |
        token->url = &tokenizer->s[tokenizer->at];
        while (tokenizer->s[tokenizer->at] != '>') {
            tokenizer->at++;
            token->url_length++;
        }
        
        tokenizer->at++;
    }
    
    return true;
}

typedef struct {
    char *text_start;
    
    char *url;
    int url_length;
} Link;

typedef struct {
    int link_count;
    Link links[16];
} Cell_Links;

char *table[][3] = {
    {
        "Page",
        "Description",
        "Source Code",
    },
    {
        "<The Royal Game of Ur|https://gelatinstudios.github.io/ur.html>", 
        "The <Royal Game of Ur|https://en.wikipedia.org/wiki/Royal_Game_of_Ur> for the <Wasm-4|https://wasm4.org/>",
        "<Github Repo|https://github.com/gelatinstudios/ur-wasm4>",
    },
    {
        "<Orrery|https://gelatinstudios.github.io/orrery.html>",
        "An <orrery|https://en.wikipedia.org/wiki/Orrery> for the browser",
        "<Github Repo|https://github.com/gelatinstudios/orrery>",
    },
    
    {
        "<This page|https://gelatinstudios.github.io>",
        "This page was written in C using <dos-like|https://mattiasgustavsson.itch.io/dos-like> and <WAjic|https://github.com/schellingb/wajic>",
        "<Github Repo|https://github.com/gelatinstudios/dos-site>",
    },
    {
        "",
        "",
        "<My Github|https://github.com/gelatinstudios>"
    },
};

typedef struct {
    Cell_Links cell_links[ArrayCount(table)][3];
    int row, col, index;
} State;

Cell_Links *get_selected_cell(State *state) {
    return &state->cell_links[state->row][state->col];
}

Link *get_selected_link(State *state) {
    return &get_selected_cell(state)->links[state->index];
}

#endif //MAIN_H
