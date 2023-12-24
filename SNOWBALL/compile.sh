clang main.c glad_gl.c -Ofast -lglfw -lm -o snowball
strip --strip-unneeded snowball
upx --lzma --best snowball