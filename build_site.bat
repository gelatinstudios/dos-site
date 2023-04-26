@echo off

pushd build
::..\wasm\node ..\wasm\wajicup ..\source\main.c -cc -O0 -no_minify -no_log -template ..\source\template.html index.html 
..\wasm\node ..\wasm\wajicup ..\source\main.c -no_log -template ..\source\template.html index.html 
popd build