#!/bin/bash

clang-format -i \
    WinCE/terminal.h \
    WinCE/terminal.c \
    Modules/_io/winceconsoleio.c \
    PC/wince_compatibility.c \
    PC/wince_compatibility.h
