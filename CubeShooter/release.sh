clang main.c glad_gl.c -Ofast -lglfw -lm -o CubeShooter
i686-w64-mingw32-gcc main.c glad_gl.c -Ofast -L. -lglfw3dll -lm -o CubeShooter.exe
strip --strip-unneeded CubeShooter
upx --lzma --best CubeShooter
strip --strip-unneeded CubeShooter.exe
upx --lzma --best CubeShooter.exe