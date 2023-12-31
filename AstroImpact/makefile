INCLUDEFLAGS = -I.

CFLAGS += $(INCLUDEFLAGS) -Ofast -DSEIR_RAND -D_REENTRANT

ifeq ($(debug), true)
	CFLAGS += -DVERBOSE -DDEBUG_GL -ggdb3
endif

ifeq ($(test), true)
	CFLAGS += -DFAST_START
endif

LDFLAGS = -lm

DEPS = $(shell $(CC) $(INCLUDEFLAGS) -MM -MT $(1).o $(1).c | sed -z 's/\\\n //g')

.PHONY: all clean cleanall release
all: bin bin/fat

bin:
	mkdir bin

$(call DEPS,client/main)
	$(CC) $(CFLAGS) -c $< -o $@

$(call DEPS,client/glad_gl)
	$(CC) $(CFLAGS) -c $< -o $@

# processing models.c is too slow, hardcode
#$(call DEPS,assets/models)
#	$(CC) $(CFLAGS) -c $< -o $@
assets/client_models.o: assets/models.c assets/models.h
	$(CC) $(CFLAGS) -c $< -o $@

assets/images.o: assets/images.c assets/images.h
	$(CC) $(CFLAGS) -c $< -o $@

$(call DEPS,inc/vec)
	$(CC) $(CFLAGS) -c $< -o $@

#$(call DEPS,inc/protocol)
#	$(CC) $(CFLAGS) -c $< -o $@

inc/munmap.o: inc/munmap.asm
	nasm -f elf64 $< -o $@

inc/real_syscall.o: inc/real_syscall.asm
	nasm -f elf64 $< -o $@

bin/fat: client/main.o client/glad_gl.o assets/client_models.o assets/images.o inc/vec.o
	$(CC) -L/app/lib $^ $(LDFLAGS) -lglfw -pthread -o $@


$(call DEPS,server/main)
	$(CC) $(CFLAGS) -c $< -o $@

$(call DEPS,server/utils)
	$(CC) $(CFLAGS) -c $< -o $@

server/sleep.o: server/sleep.asm
	nasm -f elf64 $< -o $@

assets/server_models.o: assets/models.c assets/models.h
	$(CC) $(CFLAGS) -c $< -o $@ -DEXO_VERTICES_ONLY


bin/fatserver: server/main.o server/utils.o server/sleep.o assets/server_models.o inc/vec.o inc/munmap.o inc/real_syscall.o
	$(CC) $^ $(LDFLAGS) -o $@ -static


clean:
	$(RM) bin/fat bin/fatserver bin/fat.deb client/*.o server/*.o inc/*.o bin/*.upx deb/usr/games/*

cleanall: clean
	$(RM) assets/*.o
	$(RM) website/html/index.html
	$(RM) website/html/index.js
	$(RM) website/html/index.wasm

release: bin/fat bin/fatserver
	strip --strip-unneeded bin/fat
	upx --lzma --best bin/fat
	strip --strip-unneeded bin/fatserver
	upx --lzma --best bin/fatserver
	cp flan deb/usr/games/flan
	cp bin/fat deb/usr/games/fat
	cp bin/fatserver deb/usr/games/fatserver

deb: release
	mkdir -p deb/usr/games
	dpkg-deb --build deb bin/fat.deb

deb/usr/games/fat: bin/fat
	mkdir -p deb/usr/games
	cp bin/fat deb/usr/games/fat

deb/usr/games/fatserver: bin/fatserver
	mkdir -p deb/usr/games
	cp bin/fatserver deb/usr/games/fatserver


install:
	cp bin/fat $(DESTDIR)/astroimpact

uninstall:
	rm $(DESTDIR)/astroimpact
