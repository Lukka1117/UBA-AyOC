/**
 * Utilitarios relacionados a la UI de configuración de propiedades.
 */
#pragma once

#include <stdlib.h>

/**
 * Describe una propiedad.
 *
 * Toda propiedad tiene un nombre y un lugar en memoria a actualizar cuando
 * ésta cambie. Además de eso posee una serie de callback que describen como
 * incrementarla, decrementarla, imprimirla, volverla a su valor por defecto
 * ó leerla.
 */
typedef struct {
	const char* name;
	void* value;
	void (*increase)(void*);
	void (*decrease)(void*);
	void (*print)(void*, char[32]);
	void (*zero)(void*);
	void (*scan)(void*, char[32]);
} property_t;

/**
 * Guarda el conjunto de propiedades actual en el archivo solicitado.
 */
void fquickload(property_t properties[], const char* path);

/**
 * Carga un conjunto de propiedades desde el archivo solicitado.
 */
void fquicksave(property_t properties[], const char* path);

/**
 * Guarda el conjunto de propiedades actual en `props.txt`.
 */
void quickload(property_t properties[]);

/**
 * Carga el conjunto de propiedades desde `props.txt`.
 */
void quicksave(property_t properties[]);

/**
 * Imprime la lista de propiedades en la pantalla.
 */
void properties_print(int y, property_t properties[], property_t* selected_property);

/**
 * No hace nada
 */
void nop(void* ptr);

/**
 * Definiciones de comportamiento para propiedades de tipo `float`.
 */
void float_inc(void* ptr); void float_linear_inc(void* ptr);
void float_dec(void* ptr); void float_linear_dec(void* ptr);
void float_print(void* ptr, char buf[32]);
void float_zero(void* ptr);
void float_scan(void* ptr, char buf[32]);

/**
 * Definiciones de comportamiento para propiedades de tipo `bool`.
 */
void bool_inc(void* ptr);
void bool_dec(void* ptr);
void bool_print(void* ptr, char buf[32]);
#define bool_zero bool_dec
void bool_scan(void* ptr, char buf[32]);

/**
 * Definiciones de comportamiento para propiedades de tipo `uint8_t`.
 */
void uint8_inc(void* ptr);
void uint8_dec(void* ptr);
void uint8_print(void* ptr, char buf[32]);
void uint8_zero(void* ptr);
void uint8_scan(void* ptr, char buf[32]);

/**
 * Definiciones de comportamiento para la cantidad de partículas.
 */
void emitter_count_inc(void* ptr);
void emitter_count_dec(void* ptr);
void emitter_count_print(void* ptr, char buf[32]);
#define emitter_count_zero nop
void emitter_count_scan(void* ptr, char buf[32]);

/**
 * Definiciones de tipos de propiedades.
 */
#define PROP_FLOAT         float_inc,        float_dec, float_print, float_zero, float_scan
#define PROP_LFLOAT float_linear_inc, float_linear_dec, float_print, float_zero, float_scan
#define PROP_BOOL           bool_inc,         bool_dec,  bool_print,  bool_zero,  bool_scan
#define PROP_UINT8         uint8_inc,        uint8_dec, uint8_print, uint8_zero, uint8_scan
#define PROP_NULL               NULL,             NULL,        NULL,       NULL,       NULL
