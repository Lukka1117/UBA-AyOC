/**
 * Utilitarios relacionados con capturar video.
 *
 * Requieren correr el programa utilizando el renderer por software de SDL2.
 */
#pragma once

#include <stdbool.h>
#include <SDL.h>

#include "properties.h"

/**
 * Indica si actualmente se está capturando video o no.
 */
extern bool capturing;

/**
 * Comienza/termina la captura de video.
 */
void capture_toggle(bool want_capture, bool *reset, property_t properties[]);

/**
 * Captura el frame actual de video.
 */
void capture_frame(SDL_Window *window, bool *want_capture);
