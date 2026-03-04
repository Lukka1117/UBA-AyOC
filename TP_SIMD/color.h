/**
 * Estamos usando SDL_Color en nuestro código pero no siempre queremos linkear
 * contra la SDL. Por lo tanto declaramos una copia de la estructura acá y
 * listo.
 */
#pragma once

#include <stdint.h>

/**
 * Un color RGBA. Cada canal va de 0 a 255.
 *
 * Por como está definida el ABI x86_64 los parámetros de este tipo se pasan
 * empaquetados en la parte baja de un registro.
 */
struct SDL_Color {
	uint8_t r, g, b, a;
};
