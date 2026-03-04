#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <SDL.h>
#undef main

#include "capture.h"
#include "emitter_renderer.h"
#include "emitter.h"
#include "font.h"
#include "properties.h"

#include "ejercicios.h"

#define MAX_PARTICLES  65536
#define MAX_EMIT_COUNT    64

SDL_Color color_palettes[][5] = {
	// 0: red
	{
		{ 255,   0,   0, 150 },
		{ 255,  64,   0, 150 },
		{ 255, 127,   0, 150 },
		{ 255, 127,  64, 150 },
		{ 255, 127, 127, 150 },
	},
	// 1: blue
	{
		{   0,   0, 255, 150 },
		{   0,  64, 255, 150 },
		{   0, 127, 255, 150 },
		{  64, 127, 255, 150 },
		{ 127, 127, 255, 150 },
	},
	// 2: confetti
	{
		{ 255, 240, 180, 150 },
		{ 180, 240, 255, 150 },
		{ 240, 255, 180, 150 },
		{ 255, 180, 240, 150 },
		{ 240, 180, 255, 150 },
	},
};

int main() {
	const char* titulo = "TP programacion SIMD - 2025c2";
	int a = SDL_Init(SDL_INIT_VIDEO);
	ejercicio_t* ej = &ej_c;

	SDL_Window* window = SDL_CreateWindow(titulo,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 800,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
			// | SDL_WINDOW_ALLOW_HIGHDPI
		);

	SDL_Renderer* renderer = SDL_CreateRenderer( window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

	SDL_Surface* surface_rgb = SDL_LoadBMP("texture.bmp");
	SDL_Surface* surface = SDL_ConvertSurfaceFormat(surface_rgb, SDL_PIXELFORMAT_ABGR8888, 0);
	for (int y = 0; y < surface->h; y++) {
		for (int x = 0; x < surface->w; x++) {
			uint8_t *pixels = surface->pixels;
			uint8_t *p = &pixels[(y * surface->w + x) * 4];
			if ((p[0] == 0xFF) && (p[1] == 0x00) && (p[2] == 0xFF)) {
				p[3] = 0;
			}
		}
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	font_init(renderer);

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

	emitter_t* emitter = emitter_create(MAX_PARTICLES);
	emitter->spawn_pos = (vec2_t) { 0, 90 };
	emitter->spawn_pos_spread = (vec2_t) { 50, 20 };
	emitter->spawn_vel = (vec2_t) { 0, -13 };
	emitter->spawn_vel_spread = (vec2_t) { 10, -35 };
	emitter->spawn_size = 40;
	emitter->spawn_size_spread = 70;
	emitter->spawn_colors_count = 5;
	emitter->particles_count = 4096;
	int last_palette_index = 0;
	emitter->spawn_colors = color_palettes[last_palette_index];

	SDL_Event e;
	bool quit = false;

	srand(time(NULL));

	static vec2_t center = {0, 0};

	vec2_t gravity = { 0.0, 0.5 };
	float size_a = 0.99;
	float size_b = 0.2;
	float size_c = 1;
	SDL_Color fade = { 1, 3, 7, 1 };
	vec2_t orbit_start = { -100, -100 };
	vec2_t orbit_end = { 100, 100 };
	float orbit_radius = 50;

	int    palette_index = 1;

	bool step    = false;
	bool reset   = false;
	bool emit    = true;
	bool paused  = false;
	bool want_capture = false;
	bool show_ui = true;
	bool posiciones_enabled = true;
	bool tamanios_enabled = true;
	bool colores_enabled = true;
	bool orbitar_enabled = false;
	bool arrows_enabled = false;

	property_t properties[] = {
		{ NULL, NULL, PROP_NULL }, // Signals beginning of array
		{ "count", emitter, emitter_count_inc, emitter_count_dec, emitter_count_print, emitter_count_zero, emitter_count_scan },
		{ "[DEBUG] Arrows", &arrows_enabled, PROP_BOOL },
		{ "spawn.x", &emitter->spawn_pos.x, PROP_LFLOAT },
		{ "spawn.y", &emitter->spawn_pos.y, PROP_LFLOAT },
		{ "spawn_spread.x", &emitter->spawn_pos_spread.x, PROP_LFLOAT },
		{ "spawn_spread.y", &emitter->spawn_pos_spread.y, PROP_LFLOAT },
		{ "[EJ1] Posiciones", &posiciones_enabled, PROP_BOOL },
		{ "g.x", &gravity.x, PROP_FLOAT },
		{ "g.y", &gravity.y, PROP_FLOAT },
		{ "velocity.x", &emitter->spawn_vel.x, PROP_FLOAT },
		{ "velocity.y", &emitter->spawn_vel.y, PROP_FLOAT },
		{ "velocity_spread.x", &emitter->spawn_vel_spread.x, PROP_FLOAT },
		{ "velocity_spread.y", &emitter->spawn_vel_spread.y, PROP_FLOAT },
		{ "[EJ2] Tamanios", &tamanios_enabled, PROP_BOOL },
		{ "size.a", &size_a, PROP_FLOAT },
		{ "size.b", &size_b, PROP_FLOAT },
		{ "size.c", &size_c, PROP_FLOAT },
		{ "size", &emitter->spawn_size, PROP_FLOAT },
		{ "size_spread", &emitter->spawn_size_spread, PROP_FLOAT },
		{ "[EJ3] Colores", &colores_enabled, PROP_BOOL },
		{ "palette", &palette_index, PROP_UINT8 },
		{ "fade.r", &fade.r, PROP_UINT8 },
		{ "fade.g", &fade.g, PROP_UINT8 },
		{ "fade.b", &fade.b, PROP_UINT8 },
		{ "fade.a", &fade.a, PROP_UINT8 },
		{ "[EJ4] Orbitar", &orbitar_enabled, PROP_BOOL },
		{ "orbit_start.x", &orbit_start.x, PROP_LFLOAT },
		{ "orbit_start.y", &orbit_start.y, PROP_LFLOAT },
		{ "orbit_end.x", &orbit_end.x, PROP_LFLOAT },
		{ "orbit_end.y", &orbit_end.y, PROP_LFLOAT },
		{ "orbit_radius", &orbit_radius, PROP_LFLOAT },
		{ NULL, NULL, PROP_NULL } // Signals end of array
	};

	const size_t LAST_PROPERTY = sizeof(properties) / sizeof(properties[0]) - 2;

	property_t* selected_property = &properties[1];

	while (!quit) {
		step = false;
		reset = false;

		uint64_t t = SDL_GetTicks();
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
			if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
				center.x = 0;  // para que luego se recalcule
			}
			if (e.type == SDL_KEYDOWN) {
				int sym = e.key.keysym.sym;
				bool lshift = (e.key.keysym.mod & KMOD_LSHIFT) != 0;
				bool rshift = (e.key.keysym.mod & KMOD_RSHIFT) != 0;
				if (sym == 'q') quit     = true;
				if (sym == 'e') emit    ^= true;
				if (sym == 'r') reset    = true;
				if (sym == 'p') paused  ^= true;
				if (sym == 's') step     = true;
				if (sym == 'h') show_ui ^= true;
				if (sym == 'i') ej       = ej == &ej_c ? &ej_asm : &ej_c;
				if (sym == 'c' && (lshift | rshift))
					want_capture ^= true;

				if (sym == SDLK_F5) {
					quicksave(properties);
				}
				if (sym == SDLK_F9) {
					quickload(properties);
				}

				if (sym == SDLK_DOWN) {
					selected_property++;
					if (!selected_property->name)
						selected_property = &properties[1];
				}
				if (sym == SDLK_UP) {
					selected_property--;
					if (!selected_property->name)
						selected_property = &properties[LAST_PROPERTY];
				}
				size_t times = 1;
				if (lshift) {
					times *= 10;
				}
				if (rshift) {
					times *= 10;
				}

				if (sym == SDLK_RIGHT) {
					for(size_t i = 0; i < times; i++) {
						selected_property->increase(selected_property->value);
					}
				}
				if (sym == SDLK_LEFT) {
					for(size_t i = 0; i < times; i++) {
						selected_property->decrease(selected_property->value);
					}
				}
				if (sym == SDLK_0) {
					selected_property->zero(selected_property->value);
				}
			}

			if (e.type == SDL_DROPFILE) {
				if (e.drop.file != NULL) {
					fquickload(properties, e.drop.file);
				}
			}
		}

		if (want_capture != capturing) {
			capture_toggle(want_capture, &reset, properties);
			show_ui = !want_capture;
		}

		if (reset) {
			emitter_clear_all(emitter);
		}

		if (center.x == 0) {
			SDL_Window *window = SDL_RenderGetWindow(renderer);
			int x, y;
			SDL_GetWindowSizeInPixels(window, &x, &y);
			center.x = x/2;
			center.y = y/2;
		}

		if (palette_index - 1 != last_palette_index) {
			const size_t PALETTES_COUNT = (sizeof(color_palettes) / sizeof(color_palettes[0]));
			palette_index = (palette_index + PALETTES_COUNT - 1) % PALETTES_COUNT + 1;

			last_palette_index = palette_index - 1;
			emitter->spawn_colors = color_palettes[last_palette_index];
		}

		bool advance = !paused | step;

		static size_t emit_count;

		if (advance) {
			emit_count = emit ? MAX_EMIT_COUNT : 0;
			emitter_respawn(emitter, emit_count);
			if (orbitar_enabled && ej->orbitar_hecho)
				ej->orbitar(emitter, &orbit_start, &orbit_end, orbit_radius);
			if (posiciones_enabled)
				ej->posiciones(emitter, &gravity);
			if (tamanios_enabled && ej->tamanios_hecho)
				ej->tamanios(emitter, size_a, size_b, size_c);
			if (colores_enabled && ej->colores_hecho)
				ej->colores(emitter, fade);
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		emitter_draw(emitter, renderer, texture, center);
		if (show_ui) {
			font_print(titulo, 0, 0);
			font_print("paused [p]:", 31, 1); font_print(paused      ? "enabled" : "disabled", 43, 1);
			font_print("step   [s]:", 31, 2); font_print(step        ? "step"    : "",         43, 2);
			font_print("impl   [i]:", 31, 3); font_print(ej == &ej_c ? "c"       : "asm",      43, 3);
			font_print("emit   [e]:", 31, 4); font_print(emit        ? "enabled" : "disabled", 43, 4);
			font_print("no-ui  [h]:", 31, 5);
			properties_print(1, properties, selected_property);
		}
		if (arrows_enabled)
			emitter_draw_arrows(emitter, renderer, center);
		SDL_RenderPresent(renderer);

		capture_frame(window, &want_capture);

	}

	emitter_destroy(emitter);

	return 0;
}
