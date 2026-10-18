#!/bin/zsh
gcc src/main.c src/logging.c src/ui.c src/filesys.c src/queue.c src/music.c src/decoder.c\
    -I/opt/homebrew/include/SDL2 \
    -I/opt/homebrew/opt/ffmpeg/include \
    -D_THREAD_SAFE \
    -L/opt/homebrew/lib \
    -L/opt/homebrew/opt/ffmpeg/lib \
    -lSDL2 \
    -lswresample \
    -lavformat \
    -lavcodec \
    -lavutil \
    -Wl,-framework,Cocoa \
    -Wno-incompatible-pointer-types-discards-qualifiers
