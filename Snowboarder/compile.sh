gcc main.c -I inc -Ofast -lm -lSDL2 -lGLESv2 -lEGL -o snowboarder
strip --strip-unneeded snowboarder
upx --lzma --best snowboarder