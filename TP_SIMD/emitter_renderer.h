/**
 * Funciones de rendering para nuestros emisores de partículas.
 *
 * Requieren linkear contra SDL2.
 */
#pragma once

#include <SDL.h>
#include "emitter.h"

/**
 * Dibuja las partículas de un emisor dado. Cada partícula utiliza el
 * triángulo superior izquierdo de la textura dada para visualizarse.
 */
void emitter_draw(emitter_t* emitter, SDL_Renderer* renderer, SDL_Texture* texture, vec2_t center);

/**
 * Dibuja una flecha saliendo del centro de cada partícula del emisor dado y
 * con dirección de acuerdo a la velocidad de la misma.
 */
void emitter_draw_arrows(emitter_t* emitter, SDL_Renderer* renderer, vec2_t center);
