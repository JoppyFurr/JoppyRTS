#!/bin/sh

gcc Source/*.c -o RTS -lm -lSDL2 -lSDL2_image -DJT_COMPILE_DATE=\"`date --rfc-3339=date`\" -std=c99
