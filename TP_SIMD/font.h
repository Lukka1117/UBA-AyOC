/**
 * Funciones relacionadas al código que imprime la interfaz de configuración
 * en la pantalla.
 */
#pragma once

#include <SDL.h>

/**
 * Inicializa los recursos necesarios para poner texto en la pantalla (carga
 * la tipografía, crea las texturas necesarias, etc).
 */
void font_init(SDL_Renderer *a_renderer);

/**
 * Imprime texto en la pantalla. Las coordenadas no están dadas en píxeles sino
 * en letras (dado que la tipografía es monoespaciada).
 */
void font_print(const char* text, int x, int y);
