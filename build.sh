#!/bin/sh

# I created this odd scheme whereby one can interchange different 'dict' libs.
# This makes the default 'Dict.h' that every file imports, into the hashtable.
ln -s HashDictAsDict.h Dict.h

gcc *.c        -o main.out # Just the interpreter.
gcc *.c -DTEST -o test.out # Runs a little test of the HT before running.
