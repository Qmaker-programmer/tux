#!/bin/bash

rm -rf build/ 
mkdir -p bin build

cd build
echo 'Generando Makefile...'
cmake .. && echo ' Exito.'

echo 'Compilando'

make && echo ' Exito.'

cd ..
mv build/tux bin/tux && echo './bin/tux para ejecutar'
