/**
 * Definición de la estructura a completar durante la implementación de un
 * ejercicio.
 */
#pragma once

#include "emitter.h"

/**
 * Una resolución de los ejercicios de este TP.
 *
 * Los tests de cada ejercicio dependen de que se marque el campo
 * `<ejercicio>_hecho` correspondiente. De lo contrario el tester los salteará.
 */
typedef struct {
	bool posiciones_hecho;
	bool tamanios_hecho;
	bool colores_hecho;
	bool orbitar_hecho;

	void (*posiciones)(emitter_t* emitter, vec2_t* gravedad);
	void (*tamanios)(emitter_t* emitter, float a, float b, float c);
	void (*colores)(emitter_t* emitter, SDL_Color a_restar);
	void (*orbitar)(emitter_t* emitter, vec2_t* start, vec2_t* end, float r);
} ejercicio_t;

/**
 * La resolución del TP en C.
 */
extern ejercicio_t ej_c;

/**
 * La resolución del TP en ASM.
 */
extern ejercicio_t ej_asm;
