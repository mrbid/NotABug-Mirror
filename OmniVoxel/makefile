.PHONY: all compact clean

INCLUDEFLAGS = -I.

CFLAGS += $(INCLUDEFLAGS) -Ofast

ifneq ($(native), false)
	CFLAGS += -march=native
endif

LDFLAGS =

DEPS = $(shell $(CC) $(INCLUDEFLAGS) -MM -MT $(1).o $(1).c | sed -z 's/\\\n //g')

CLIENTCFLAGS = -DNO_COMPRESSION
CLIENTLDFLAGS = -lm -lSDL2 -lGLESv2 -lEGL

all: bin/vox

bin/vox: client/main.o client/assets.o
	mkdir bin -p
	$(CC) $^ $(LDFLAGS) $(CLIENTLDFLAGS) -o $@

$(call DEPS,client/main)
$(call DEPS,client/assets)

client/%.o: client/%.c
	$(CC) $(CFLAGS) $(CLIENTCFLAGS) -c $< -o $@

compact: bin/vox
	strip --strip-unneeded bin/vox
	upx bin/vox

# `bin/vox' rather than `all', `all' will later include the server
test: bin/vox
	bin/vox

clean:
	$(RM) bin/* client/*.o
