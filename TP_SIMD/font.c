#include "font.h"

#define FONT_WIDTH   96
#define FONT_HEIGHT  96

#define GLYPH_WIDTH   6
#define GLYPH_HEIGHT 12

static SDL_Texture*  font_tex;
static SDL_Renderer* g_renderer;

static void draw_char(SDL_Renderer *renderer, unsigned char character, int cx, int cy, int scale) {
	SDL_Point cell = {
		(character-32) % (FONT_WIDTH/GLYPH_WIDTH),
		(character-32) / (FONT_WIDTH/GLYPH_WIDTH)
	};

	SDL_Rect src_rect = {
		cell.x * GLYPH_WIDTH,
		cell.y * GLYPH_HEIGHT,
		GLYPH_WIDTH,
		GLYPH_HEIGHT,
	};

	SDL_Rect dst_rect = {
		cx,
		cy,
		GLYPH_WIDTH * scale,
		GLYPH_HEIGHT * scale,
	};

	SDL_RenderCopy(renderer, font_tex, &src_rect, &dst_rect);
}

static void draw_text(SDL_Renderer *renderer, const char* text, int x, int y, int scale) {
	for (size_t i = 0; text[i] != '\0'; i++) {
		draw_char(
			renderer,
			text[i],
			(x + i) * GLYPH_WIDTH  * scale,
			 y      * GLYPH_HEIGHT * scale,
			scale
		);
	}
}

void font_load() {
	SDL_Surface* font_surf = SDL_LoadBMP("monogram.bmp");
	for (int y = 0; y < font_surf->h; y++) {
		for (int x = 0; x < font_surf->w; x++) {
			uint8_t *pixels = font_surf->pixels;
			uint8_t *p = &pixels[(y * font_surf->w + x) * 4];
			if (p[3] == 0xFF) {
				p[0] =  0xFF;
				p[1] =  0xFF;
				p[2] =  0xFF;
			}
		}
	}
	font_tex = SDL_CreateTextureFromSurface(g_renderer, font_surf);
}

void font_init(SDL_Renderer *a_renderer) {
	g_renderer = a_renderer;
	font_load();
}

void font_print(const char* text, int x, int y) {
	draw_text(g_renderer, text, x, y, 2);
}
