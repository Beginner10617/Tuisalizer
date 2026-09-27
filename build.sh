#!/bin/zsh
gcc src/main.c src/logging.c src/ui.c \
    -I/opt/homebrew/include/SDL2 \
    -I/opt/homebrew/opt/ffmpeg/include \
    -D_THREAD_SAFE \
    -L/opt/homebrew/lib \
    -L/opt/homebrew/opt/ffmpeg/lib \
    -lSDL2 \
    -lavformat \
    -lavcodec \
    -lavutil \
    -Wl,-framework,Cocoa
