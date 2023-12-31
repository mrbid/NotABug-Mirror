make clean
make -j`nproc` all
clear;clear
./bin/fat $((`date +%s`+7))
