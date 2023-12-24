clang main.c glad_gl.c -Ofast -lglfw -lm -o spaceminer
strip --strip-unneeded spaceminer
upx --lzma --best spaceminer
./spaceminer
