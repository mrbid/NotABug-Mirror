i686-w64-mingw32-gcc main.c glad_gl.c -Ofast -I inc -L. -lglfw3dll -lm -o SpaceMiner.exe
strip --strip-unneeded SpaceMiner.exe
upx --lzma --best SpaceMiner.exe