@echo off

pushd build
tcc ..\source\main.c
::..\wasm\node ..\wasm\wajicup ..\source\main.c site.o
popd build