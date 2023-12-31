make -j`nproc` test=true
./bin/fat $((`date +%s`+6))
