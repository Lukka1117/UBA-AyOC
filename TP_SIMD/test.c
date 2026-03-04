#include <math.h>
#include <stdio.h>

#include "test.h"

#include "color.h"
#include "ejercicios.h"
#include "emitter.h"

#ifdef USE_C
#  define ej ej_c
#elif USE_ASM
#  define ej ej_asm
#else
# error "Tenés que usar C o ASM che!"
#endif

static
void spawn(emitter_t* emitter, size_t i, float pos_x, float pos_y, float vel_x, float vel_y, SDL_Color color, float size) {
	emitter->particles_pos[i].x = pos_x;
	emitter->particles_pos[i].y = pos_y;
	emitter->particles_vel[i].x = vel_x;
	emitter->particles_vel[i].y = vel_y;
	emitter->particles_color[i] = color;
	emitter->particles_size[i] = size;
}

#ifdef USE_C
#  define ej_posiciones ej_posiciones_c
#  define ej_tamanios    ej_tamanios_c
#  define ej_colores    ej_colores_c
#elif USE_ASM
#  define ej_posiciones ej_posiciones_asm
#  define ej_tamanios    ej_tamanios_asm
#  define ej_colores    ej_colores_asm
#else
# error "Tenés que usar C o ASM che!"
#endif

#ifdef RECORD_SNAPSHOTS
#  define VALIDATE(emitter, i) record_snapshot(test__name, (i), (emitter))
#else /* Just do validation */
#  define VALIDATE(emitter, i) validate_snapshot(test__name, test__fallo, (i), (emitter))
#endif

void record_snapshot(const char* name, int snapshot_id, const emitter_t* emitter) {
	char filename[1024] = { 0 };
	snprintf(filename, 1024, "snapshots/%s.%d.data", name, snapshot_id);
	FILE* snapshot = fopen(filename, "w+");
	fprintf(snapshot, "count: %ld\n", emitter->particles_count);
	for (size_t i = 0; i < emitter->particles_count; i++) {
		fprintf(snapshot, "particle[%ld].pos: (%f; %f)\n", i, emitter->particles_pos[i].x, emitter->particles_pos[i].y);
		fprintf(snapshot, "particle[%ld].vel: (%f; %f)\n", i, emitter->particles_vel[i].x, emitter->particles_vel[i].y);
		fprintf(snapshot, "particle[%ld].size: %f\n", i, emitter->particles_size[i]);
		fprintf(snapshot, "particle[%ld].color: #%.2X%.2X%.2X%.2X\n", i,
			emitter->particles_color[i].r,
			emitter->particles_color[i].g,
			emitter->particles_color[i].b,
			emitter->particles_color[i].a);
	}
	fclose(snapshot);
}

void validate_snapshot(const char* name, bool* failed, int snapshot_id, const emitter_t* emitter) {
	bool failed_here = false;
	char filename[1024] = { 0 };
	snprintf(filename, 1024, "snapshots/%s.%d.data", name, snapshot_id);
	FILE* snapshot = fopen(filename, "r");

	int matched;
	size_t count;
	matched = fscanf(snapshot, "count: %ld\n", &count);
	if (matched != 1) exit(1);
	if (count != emitter->particles_count) {
		if (!*failed) {
			printf(" " FAILED "\n");
		}
		if (!failed_here) {
			printf("    Al validar el snapshot %d de %s:\n", snapshot_id, name);
		}
		printf("      - La cantidad de partículas es distinta a la esperada\n");
		*failed = failed_here = true;
		return;
	}

	for (size_t i = 0; i < emitter->particles_count; i++) {
		size_t particle_i;
		vec2_t pos, vel;
		float size;
		SDL_Color color;

		matched = fscanf(snapshot, "particle[%ld].pos: (%f; %f)\n", &particle_i, &pos.x, &pos.y);
		if (matched != 3 || particle_i != i) exit(1);
		matched = fscanf(snapshot, "particle[%ld].vel: (%f; %f)\n", &particle_i, &vel.x, &vel.y);
		if (matched != 3 || particle_i != i) exit(1);
		matched = fscanf(snapshot, "particle[%ld].size: %f\n", &particle_i, &size);
		if (matched != 2 || particle_i != i) exit(1);
		matched = fscanf(snapshot, "particle[%ld].color: #%2hhX%2hhX%2hhX%2hhX\n", &particle_i,
			&color.r, &color.g, &color.b, &color.a);
		if (matched != 5 || particle_i != i) exit(1);

		if (0.0001 < fabs(pos.x - emitter->particles_pos[i].x) || 0.0001 < fabs(pos.y - emitter->particles_pos[i].y)) {
			if (!*failed) {
				printf(" " FAILED "\n");
			}
			if (!failed_here) {
				printf("    Al validar el snapshot %d de %s:\n", snapshot_id, name);
			}
			printf("      - La partícula %ld debería estar en (%f; %f) pero está en (%f; %f)\n",
				i,
				pos.x, pos.y,
				emitter->particles_pos[i].x, emitter->particles_pos[i].y);
			*failed = failed_here = true;
		}

		if (0.0001 < fabs(vel.x - emitter->particles_vel[i].x) || 0.0001 < fabs(vel.y - emitter->particles_vel[i].y)) {
			if (!*failed) {
				printf(" " FAILED "\n");
			}
			if (!failed_here) {
				printf("    Al validar el snapshot %d de %s:\n", snapshot_id, name);
			}
			printf("      - La partícula %ld debería tener velocidad (%f; %f) pero su velocidad es (%f; %f)\n",
				i,
				vel.x, vel.y,
				emitter->particles_vel[i].x, emitter->particles_vel[i].y);
			*failed = failed_here = true;
		}

		if (0.0001 < fabs(size - emitter->particles_size[i])) {
			if (!*failed) {
				printf(" " FAILED "\n");
			}
			if (!failed_here) {
				printf("    Al validar el snapshot %d de %s:\n", snapshot_id, name);
			}
			printf("      - La partícula %ld debería tener tamaño %f pero su tamaño es %f\n",
				i, size, emitter->particles_size[i]);
			*failed = failed_here = true;
		}

		if (color.r != emitter->particles_color[i].r || color.g != emitter->particles_color[i].g || color.b != emitter->particles_color[i].b || color.a != emitter->particles_color[i].a) {
			if (!*failed) {
				printf(" " FAILED "\n");
			}
			if (!failed_here) {
				printf("    Al validar el snapshot %d de %s:\n", snapshot_id, name);
			}
			printf("      - La partícula %ld debería tener color #%.2X%.2X%.2X%.2X pero su color es #%.2X%.2X%.2X%.2X\n",
				i,
				color.r, color.g, color.b, color.a,
				emitter->particles_color[i].r, emitter->particles_color[i].g, emitter->particles_color[i].b, emitter->particles_color[i].a);
			*failed = failed_here = true;
		}
	}
	fclose(snapshot);
}

static emitter_t *new_test_emitter() {
	emitter_t* emitter = emitter_create(8);
	spawn(emitter, 0, 0.00,   0.00,  5.00,  5.00, (SDL_Color){   0,   0,   0, 255 }, 1000.00);
	spawn(emitter, 1, 0.00,   0.00,  5.00, -5.00, (SDL_Color){   0,   0, 255, 255 },  100.00);
	spawn(emitter, 2, 0.00,   0.00, -5.00,  5.00, (SDL_Color){   0, 255,   0, 255 },   10.00);
	spawn(emitter, 3, 0.00,   0.00, -5.00, -5.00, (SDL_Color){   0, 255, 255, 255 },    1.00);
	spawn(emitter, 4, 0.32, 100.10,  0.50, -0.20, (SDL_Color){ 255,   0,   0, 255 },    0.10);
	spawn(emitter, 5, 1.23,   4.56, -0.10,  9.10, (SDL_Color){ 255,   0, 255, 255 },    0.01);
	spawn(emitter, 6, 7.89,   0.12,  8.50,  7.30, (SDL_Color){ 255, 255,   0, 255 },    7.23);
	spawn(emitter, 7, 3.45,   6.78,  0.00,  0.00, (SDL_Color){ 255, 255, 255, 255 },    5.00);
	return emitter;
}

static emitter_t *new_test_emitter_tamanios(uint8_t size_mask) {
	emitter_t* emitter = emitter_create(8);
	SDL_Color col = { 0, 0, 0, 255 };
	for (size_t i = 0; i < 8; i++) {
		spawn(emitter, i, 0.00, 0.00, 5.00, 5.00, col,
		      (((size_mask & (1 << i)) > 0) ? 10.0 : 3.0) );
	}
	return emitter;
}

TEST(posiciones_1, ej.posiciones_hecho) {
	emitter_t* emitter = new_test_emitter();

	ej.posiciones(emitter, &(vec2_t) { 5.00, 5.00 });
	VALIDATE(emitter, 1);
	ej.posiciones(emitter, &(vec2_t) { 5.00, 5.00 });
	VALIDATE(emitter, 2);
	ej.posiciones(emitter, &(vec2_t) { 5.00, 5.00 });
	VALIDATE(emitter, 3);
	ej.posiciones(emitter, &(vec2_t) { 5.00, 5.00 });
	VALIDATE(emitter, 4);
	ej.posiciones(emitter, &(vec2_t) { 5.00, 5.00 });
	VALIDATE(emitter, 5);

	for (int i = 0; i < 255; i++) {
		ej.posiciones(emitter, &(vec2_t) { 5.00, 5.00 });
	}
	VALIDATE(emitter, 260);

	emitter_destroy(emitter);
}

TEST(tamanios, ej.tamanios_hecho) {
	emitter_t* emitter = new_test_emitter();

	float a_decay1 = 0.99;
	float b_decay2 = 2.0;
	float c_cutoff = 3.0;
	ej.tamanios(emitter, a_decay1, b_decay2, c_cutoff);
	VALIDATE(emitter, 1);
	ej.tamanios(emitter, a_decay1, b_decay2, c_cutoff);
	VALIDATE(emitter, 2);
	ej.tamanios(emitter, a_decay1, b_decay2, c_cutoff);
	VALIDATE(emitter, 3);
	ej.tamanios(emitter, a_decay1, b_decay2, c_cutoff);
	VALIDATE(emitter, 4);
	ej.tamanios(emitter, a_decay1, b_decay2, c_cutoff);
	VALIDATE(emitter, 5);

	for (int i = 0; i < 255; i++) {
		ej.tamanios(emitter, a_decay1, b_decay2, c_cutoff);
	}
	VALIDATE(emitter, 260);

	emitter_destroy(emitter);
}

TEST(tamanios_cutoff_1, ej.tamanios_hecho) {
	emitter_t* emitter = new_test_emitter_tamanios(0b00001111);
	size_t i = 0;

	float a_decay1 = 0.99;
	float b_decay2 = 2.0;
	float c_cutoff = 5.0;
	ej.tamanios(emitter, a_decay1, b_decay2, c_cutoff);
	VALIDATE(emitter, 1);

	emitter_destroy(emitter);
}

TEST(tamanios_cutoff_2, ej.tamanios_hecho) {
	emitter_t* emitter = new_test_emitter_tamanios(0b00111100);
	size_t i = 0;

	float a_decay1 = 0.99;
	float b_decay2 = 2.0;
	float c_cutoff = 5.0;
	ej.tamanios(emitter, a_decay1, b_decay2, c_cutoff);
	VALIDATE(emitter, 1);

	emitter_destroy(emitter);
}

TEST(tamanios_cutoff_3, ej.tamanios_hecho) {
	emitter_t* emitter = new_test_emitter_tamanios(0b01011010);
	size_t i = 0;

	float a_decay1 = 0.99;
	float b_decay2 = 2.0;
	float c_cutoff = 5.0;
	ej.tamanios(emitter, a_decay1, b_decay2, c_cutoff);
	VALIDATE(emitter, 1);

	emitter_destroy(emitter);
}

TEST(colores, ej.colores_hecho) {
	emitter_t* emitter = new_test_emitter();

	SDL_Color a_restar = {  33, 15, 255, 1 };
	ej.colores(emitter, a_restar);
	VALIDATE(emitter, 1);
	ej.colores(emitter, a_restar);
	VALIDATE(emitter, 2);
	ej.colores(emitter, a_restar);
	VALIDATE(emitter, 3);
	ej.colores(emitter, a_restar);
	VALIDATE(emitter, 4);
	ej.colores(emitter, a_restar);
	VALIDATE(emitter, 5);

	for (int i = 0; i < 255; i++) {
		ej.colores(emitter, a_restar);
	}
	VALIDATE(emitter, 260);

	emitter_destroy(emitter);

}

TEST(orbitar, ej.orbitar_hecho) {
	emitter_t* emitter = new_test_emitter();

	vec2_t start = { -0.4, -0.4 };
	vec2_t end   = { 0.3, 0.1 };
	float radius = 0.5;

	ej.orbitar(emitter, &start, &end, radius);
	VALIDATE(emitter, 1);
	ej.orbitar(emitter, &start, &end, radius);
	ej.orbitar(emitter, &start, &end, radius);
	ej.orbitar(emitter, &start, &end, radius);
	ej.orbitar(emitter, &start, &end, radius);
	VALIDATE(emitter, 5);

	for (int i = 0; i < 255; i++) {
		ej.orbitar(emitter, &start, &end, radius);
	}
	VALIDATE(emitter, 260);


	emitter_destroy(emitter);
}

TEST(orbitar_todo_dentro, ej.orbitar_hecho) {
	emitter_t* emitter = new_test_emitter();

	vec2_t start = { -100, -100 };
	vec2_t end   = {  100,  100 };
	float radius = 100;

	ej.orbitar(emitter, &start, &end, radius);
	VALIDATE(emitter, 1);
	ej.orbitar(emitter, &start, &end, radius);
	ej.orbitar(emitter, &start, &end, radius);
	ej.orbitar(emitter, &start, &end, radius);
	ej.orbitar(emitter, &start, &end, radius);
	VALIDATE(emitter, 5);

	for (int i = 0; i < 255; i++) {
		ej.orbitar(emitter, &start, &end, radius);
	}
	VALIDATE(emitter, 260);


	emitter_destroy(emitter);
}

TEST(orbitar_todo_fuera, ej.orbitar_hecho) {
	emitter_t* emitter = new_test_emitter();

	vec2_t start = { 10, 15 };
	vec2_t end   = { 20, 25 };
	float radius = 2;

	ej.orbitar(emitter, &start, &end, radius);
	VALIDATE(emitter, 1);
	ej.orbitar(emitter, &start, &end, radius);
	ej.orbitar(emitter, &start, &end, radius);
	ej.orbitar(emitter, &start, &end, radius);
	ej.orbitar(emitter, &start, &end, radius);
	VALIDATE(emitter, 5);

	for (int i = 0; i < 255; i++) {
		ej.orbitar(emitter, &start, &end, radius);
	}
	VALIDATE(emitter, 260);


	emitter_destroy(emitter);
}

int main(void) {
#ifndef SKIP_EJ1
	printf("== Ejercicio 1 ==\n");
	posiciones_1();
#endif

#ifndef SKIP_EJ2
	printf("== Ejercicio 2 ==\n");
	tamanios();
	tamanios_cutoff_1();
	tamanios_cutoff_2();
	tamanios_cutoff_3();
#endif

#ifndef SKIP_EJ3
	printf("== Ejercicio 3 ==\n");
	colores();
#endif

#ifndef SKIP_EJ4
	printf("== Ejercicio 4 (opcional) ==\n");
	orbitar();
	orbitar_todo_dentro();
	orbitar_todo_fuera();
#endif

	printf("\n");
	return test__count != test__succeed__count;
}
