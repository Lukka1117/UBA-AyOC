UNAME_SM := $(shell uname -sm)
ifneq ($(UNAME_SM),Darwin arm64)
    # for regular linux build
    NASM_FORMAT = elf64
    SDL2_CONFIG = sdl2-config
    LDFLAGS += -z noexecstack
else
    # for macOS aarch64: x86_64 build
    CC = clang -arch x86_64
    NASM_FORMAT = macho64
    SDL2_CONFIG = /usr/local/bin/sdl2-config
    NASMFLAGS += --prefix _
endif

CFLAGS_main += $(CFLAGS) -Wall -Wextra -Wno-unused -std=c11 -ggdb -Og $$($(SDL2_CONFIG) --cflags)
CFLAGS_test += $(CFLAGS) -Wall -Wextra -Wno-unused -std=c11 -ggdb -Og
LDFLAGS_main = $(LDFLAGS) -lm $$($(SDL2_CONFIG) --libs)
LDFLAGS_test = $(LDFLAGS) -lm
VALGRIND = valgrind --show-reachable=yes --leak-check=full --error-exitcode=1 --track-origins=yes 

all: main test_c test_asm

main: main.c emitter.c emitter_renderer.c ejercicios.c ejercicios_asm.o font.c capture.c properties.c
	$(CC) $^ $(CFLAGS_main) $(LDFLAGS_main) -o $@

ejercicios_asm.o: ejercicios.asm
	nasm -f $(NASM_FORMAT) -F dwarf $(NASMFLAGS) $^ -o $@

test_c: test.c emitter.c ejercicios.c
	$(CC) $^ $(CFLAGS_test) -DUSE_C $(LDFLAGS_test) -o $@

test_asm: test.c emitter.c ejercicios_asm.o
	$(CC) $^ $(CFLAGS_test) -DUSE_ASM $(LDFLAGS_test) -o $@

grader: test.c emitter.c ejercicios_asm.o
	$(CC) $^ $(CFLAGS_test) -DUSE_ASM -DSKIP_EJ4 $(LDFLAGS_test) -o $@

test_recorder: test.c emitter.c ejercicios.c
	$(CC) $^ $(CFLAGS_test) -DUSE_C -DRECORD_SNAPSHOTS $(LDFLAGS_test) -o $@

run: main
	./main

run_sw: main
	ffmpeg -version > /dev/null
	SDL_RENDER_DRIVER=software ./main

run_c: test_c
	./test_c

run_asm: test_asm
	./test_asm

run_grader: grader
	./grader

valgrind_c: test_c
	$(VALGRIND) ./test_c
	echo "No se detectaron errores de memoria"

valgrind_asm: test_asm
	$(VALGRIND) ./test_asm
	echo "No se detectaron errores de memoria"

valgrind_grader: grader
	$(VALGRIND) ./grader
	echo "No se detectaron errores de memoria"

# Targets usados por el tester

test_c_ej1: test.c emitter.c ejercicios.c
	$(CC) $^ $(CFLAGS_test) -DUSE_C -DSKIP_EJ2 -DSKIP_EJ3 -DSKIP_EJ4 $(LDFLAGS_test) -o $@

test_c_ej2: test.c emitter.c ejercicios.c
	$(CC) $^ $(CFLAGS_test) -DUSE_C -DSKIP_EJ1 -DSKIP_EJ3 -DSKIP_EJ4 $(LDFLAGS_test) -o $@

test_c_ej3: test.c emitter.c ejercicios.c
	$(CC) $^ $(CFLAGS_test) -DUSE_C -DSKIP_EJ1 -DSKIP_EJ2 -DSKIP_EJ4 $(LDFLAGS_test) -o $@

test_c_ej4: test.c emitter.c ejercicios.c
	$(CC) $^ $(CFLAGS_test) -DUSE_C -DSKIP_EJ1 -DSKIP_EJ2 -DSKIP_EJ3 $(LDFLAGS_test) -o $@

test_asm_ej1: test.c emitter.c ejercicios_asm.o
	$(CC) $^ $(CFLAGS_test) -DUSE_ASM -DSKIP_EJ2 -DSKIP_EJ3 -DSKIP_EJ4 $(LDFLAGS_test) -o $@

test_asm_ej2: test.c emitter.c ejercicios_asm.o
	$(CC) $^ $(CFLAGS_test) -DUSE_ASM -DSKIP_EJ1 -DSKIP_EJ3 -DSKIP_EJ4 $(LDFLAGS_test) -o $@

test_asm_ej3: test.c emitter.c ejercicios_asm.o
	$(CC) $^ $(CFLAGS_test) -DUSE_ASM -DSKIP_EJ1 -DSKIP_EJ2 -DSKIP_EJ4 $(LDFLAGS_test) -o $@

test_asm_ej4: test.c emitter.c ejercicios_asm.o
	$(CC) $^ $(CFLAGS_test) -DUSE_ASM -DSKIP_EJ1 -DSKIP_EJ2 -DSKIP_EJ3 $(LDFLAGS_test) -o $@

run_c_ej1: test_c_ej1
	./test_c_ej1
	$(VALGRIND) ./test_c_ej1

run_c_ej2: test_c_ej2
	./test_c_ej2
	$(VALGRIND) ./test_c_ej2

run_c_ej3: test_c_ej3
	./test_c_ej3
	$(VALGRIND) ./test_c_ej3

run_c_ej4: test_c_ej4
	./test_c_ej4
	$(VALGRIND) ./test_c_ej4

run_asm_ej1: test_asm_ej1
	./test_asm_ej1
	$(VALGRIND) ./test_asm_ej1

run_asm_ej2: test_asm_ej2
	./test_asm_ej2
	$(VALGRIND) ./test_asm_ej2

run_asm_ej3: test_asm_ej3
	./test_asm_ej3
	$(VALGRIND) ./test_asm_ej3

run_asm_ej4: test_asm_ej4
	./test_asm_ej4
	$(VALGRIND) ./test_asm_ej4

clean:
	rm -f main test_c test_asm test_{c,asm}_ej{1,2,3,4} grader test_recorder ejercicios_asm.o

.PHONY: all clean run run_sw run_c run_asm run_grader valgrind_c valgrind_asm valgrind_grader run_c_ej1 run_c_ej2 run_c_ej3 run_c_ej4 run_asm_ej1 run_asm_ej2 run_asm_ej3 run_asm_ej4
