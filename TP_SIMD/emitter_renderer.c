#include "emitter.h"
#include "emitter_renderer.h"

#define MAX_PARTICLES_PER_DRAW_CALL (1 << 16)

void emitter_draw(emitter_t* emitter, SDL_Renderer* renderer, SDL_Texture* texture, vec2_t center) {
	size_t count = emitter->particles_count;

	const size_t vec2_stride = sizeof(vec2_t);
	const size_t color_stride = sizeof(SDL_Color);
	const size_t uv_stride = sizeof(float) * 2;

	static float vertices[6 * MAX_PARTICLES_PER_DRAW_CALL];
	static SDL_Color colors[3 * MAX_PARTICLES_PER_DRAW_CALL];
	static float uvs[6 * MAX_PARTICLES_PER_DRAW_CALL];

	size_t i = 0;

	for (size_t j = 0; j < count; j++) {
		if (emitter->particles_size[j] <= 0) {
			continue;
		}

		vertices[i*6+0] = center.x + emitter->particles_pos[j].x + emitter->particles_size[j] * -0.25f;
		vertices[i*6+1] = center.y + emitter->particles_pos[j].y + emitter->particles_size[j] * -0.25f;
		vertices[i*6+2] = center.x + emitter->particles_pos[j].x + emitter->particles_size[j] *  0.75f;
		vertices[i*6+3] = center.y + emitter->particles_pos[j].y + emitter->particles_size[j] * -0.25f;
		vertices[i*6+4] = center.x + emitter->particles_pos[j].x + emitter->particles_size[j] * -0.25f;
		vertices[i*6+5] = center.y + emitter->particles_pos[j].y + emitter->particles_size[j] *  0.75f;

		colors[i*3+0] = emitter->particles_color[j];
		colors[i*3+1] = emitter->particles_color[j];
		colors[i*3+2] = emitter->particles_color[j];

		uvs[i*6+0] = 0.0f;
		uvs[i*6+1] = 0.0f;
		uvs[i*6+2] = 1.0f;
		uvs[i*6+3] = 0.0f;
		uvs[i*6+4] = 0.0f;
		uvs[i*6+5] = 1.0f;
		i++;
		if (i == MAX_PARTICLES_PER_DRAW_CALL) {
			SDL_RenderGeometryRaw(
				renderer,
				texture,
				vertices, vec2_stride,
				colors, color_stride,
				uvs, uv_stride,
				i * 3,
				NULL, 0, 0);
			i = 0;
		}
	}

	if (i != 0) {
		SDL_RenderGeometryRaw(
			renderer,
			texture,
			vertices, vec2_stride,
			colors, color_stride,
			uvs, uv_stride,
			i * 3,
			NULL, 0, 0);
	}
}

void emitter_draw_arrows(emitter_t* emitter, SDL_Renderer* renderer, vec2_t center) {
	size_t count = emitter->particles_count;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 127);
	for (size_t i = 0; i < count; i++) {
		if (emitter->particles_size[i] <= 0) {
			continue;
		}
		vec2_t pos = emitter->particles_pos[i];
		vec2_t vel = emitter->particles_vel[i];
		float size = emitter->particles_size[i];
		pos.x += center.x + size / 4;
		pos.y += center.y + size / 4;
		float len = sqrtf(vel.x * vel.x + vel.y * vel.y);
		float end_x = pos.x + vel.x / len * size / 4;
		float end_y = pos.y + vel.y / len * size / 4;
		if (SDL_RenderDrawLineF(renderer, pos.x, pos.y, end_x, end_y)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", SDL_GetError());
		}
	}
}
