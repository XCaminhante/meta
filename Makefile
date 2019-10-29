CFLAGS := -Os -D_FILE_OFFSET_BITS=64 -std=gnu99 -D_GNU_SOURCE -Wall -Wextra -pedantic -flto

all: evolve

%: %.c

%.c: %.txt meta
	./meta $< $@

meta2.c: meta.txt support.h
	./meta meta.txt meta2.c

meta3.c: meta2
	./meta2 meta.txt meta3.c

meta4.c: meta3
	./meta3 meta.txt meta4.c

bootstrap: meta2.c

metacompile: bootstrap meta4.c
	diff meta3.c meta4.c

evolve: metacompile
	mv meta3.c meta.c
	mv meta3 meta
	rm meta?.c meta? -f

clean:
	rm meta? meta?.[co] -f
