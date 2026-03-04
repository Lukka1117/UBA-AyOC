/**
 * Estructura el emisor y funciones públicas asociadas al mismo. No incluye
 * funciones relacionadas al rendering del emisor. El API de rendering se
 * encuentra en `emitter_renderer.h` y requiere linkear contra SDL2 para ser
 * usado.
 */
#pragma once

#include <stddef.h>

/**
 * Un color RGBA. Cada canal va de 0 a 255.
 *
 * Como no todos los usuarios de este header linkean contra SDL2 esta
 * definición nos permite usar la de <SDL.h> en unos archivos y la de
 * "color.h" en otros. Esto funciona _de milagro_ porque el struct es igual en
 * ambas definiciones.
 */
typedef struct SDL_Color SDL_Color;

/**
 * Un vector en dos dimensiones.
 *
 * Ambas coordenadas son números flotantes de 32 bits.
 */
typedef struct {
    float x, y;         //cada float son 4 bytes, struct ocupa 8
} vec2_t;

/**
 * La descripción del nuestros emisores de partículas. Posee tanto los
 * parámetros de creación de nuevas partículas como la lista de partículas
 * ya creadas.
 *
 * Los campos `<propiedad>_spread` configuran la variabilidad del atributo en
 * cuestión. Los campos `<propiedad>` el valor en el centro (es decir, si
 * `spawn_pos.x = 5` y `spawn_pos_spread.x = 3` entonces las partículas se
 * crearán con coordenadas horizontales de entre 3.5 y 6.5`).
 *
 * - `particles_count`: Cantidad máxima de partículas a procesar.
 * - `particles_capacity`: Cantidad máxima de partículas para las que se
 *                         reservó memoria.
 * - `particles_pos`: Posición (x, y) de cada partícula.
 * - `particles_color`: Color (R, G, B, A) de cada partícula.
 * - `particles_size`: Tamaño (r) de cada partícula.
 * - `particles_vel`: Velocidad (v_x, v_y) de cada partícula.
 */
typedef struct {
    // Spawn config
    vec2_t spawn_pos;           //8 bytes, offset 0
    vec2_t spawn_pos_spread;    //8 byte, offset 8
    vec2_t spawn_vel;           // 8 bytes, offset 16
    vec2_t spawn_vel_spread;    //8 bytes, offset 24
    float spawn_size;           //4 bytes, offset 32
    float spawn_size_spread;    //4 btyes, offset 36
    size_t spawn_colors_count;  //8 bytes, offset 40
    const SDL_Color* spawn_colors;  //8 bytes, offset 48
    // Particles
    size_t     particles_count;     //8 bytes, offset 56
    size_t     particles_capacity;  //8 bytes, offset 64
    vec2_t*    particles_pos;       //8 bytes, offset 72
    SDL_Color* particles_color;     //8 bytes, offset 80
    float*     particles_size;      //8 bytes, offset 88
    vec2_t*    particles_vel;       //8 bytes, offset 96
} emitter_t;    //emitter Size 104

/**
 * Elimina las partículas innecesarias (muy chicas, muy transparentes, muy
 * lejanas) y las reemplaza por nuevas partículas. Crea un máximo de
 * `spawn_limit` partículas.
 */
void emitter_respawn(emitter_t* emitter, size_t spawn_limit);

/**
 * Reemplaza todas las partículas por nuevas partículas.
 */
void emitter_spawn_all(emitter_t* emitter);

/**
 * Crea un nuevo emisor de partículas que puede administrar hasta
 * `max_particles` partículas.
 */
emitter_t* emitter_create(size_t max_particles);

/**
 * Destruye un emisor de partículas y libera todos los recursos asociados a él.
 */
void emitter_destroy(emitter_t* emitter);

/**
 * Elimina todas las partículas del emisor dado sin reemplazarlas por
 * partículas nuevas.
 */
void emitter_clear_all(emitter_t* emitter);
