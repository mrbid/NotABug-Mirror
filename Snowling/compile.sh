gcc main.c -I inc -Ofast -lm -lSDL2 -lGLESv2 -lEGL -o snowling
strip --strip-unneeded snowling
upx --lzma --best snowling