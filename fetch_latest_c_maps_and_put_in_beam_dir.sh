#!/bin/bash

BEAM_DIR=erts/emulator/beam

rm -rf c_maps

git clone git://github.com/davidklaftenegger/c_maps.git

cp c_maps/*.c $BEAM_DIR
cp c_maps/*.h $BEAM_DIR
