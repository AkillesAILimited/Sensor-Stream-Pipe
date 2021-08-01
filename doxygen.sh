#!/bin/bash

doxygen doxyfile
cd docs/latex
make 
make
cp -fv refman.pdf ../../ssp.pdf
cd ../..
