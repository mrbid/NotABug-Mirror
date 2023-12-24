i686-w64-mingw32-gcc main.c glad_gl.c -Ofast -I inc -L. -lglfw3dll -lm -o Snowball.exe
strip --strip-unneeded Snowball.exe
upx --lzma --best Snowball.exe