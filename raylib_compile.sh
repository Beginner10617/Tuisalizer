#!/bin/zsh
clang main.c audio_interface_desktop.c logging.c\
  -Iraylib/src \
  -Lraylib/build/raylib \
  -lraylib \
  -framework OpenGL \
  -framework Cocoa \
  -framework IOKit \
  -framework CoreVideo \
  -framework QuartzCore \
  -o build/app
