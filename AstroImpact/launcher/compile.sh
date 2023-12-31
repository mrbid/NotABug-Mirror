qmake flan.pro -spec linux-clang
make
strip --strip-unneeded flan
upx --lzma --best flan