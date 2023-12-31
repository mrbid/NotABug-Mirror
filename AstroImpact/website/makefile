all:
	mkdir -p html
	emcc ../assets/models.c main.c -O3 --closure 1 -s FILESYSTEM=0 -s USE_SDL=2 -s ENVIRONMENT=web -s TOTAL_MEMORY=32MB -o html/index.html --shell-file t.html

run:
	emrun --browser chrome html/index.html

clean:
	rm -f html/index.html
	rm -f html/index.js
	rm -f html/index.wasm
