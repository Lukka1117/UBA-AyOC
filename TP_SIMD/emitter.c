#define _DEFAULT_SOURCE
#include <stdint.h>
#include <stdlib.h>

#include "color.h"
#include "emitter.h"

static
uint64_t iabs(int64_t v) {
	return v < 0 ? -v : v;
}

static
float randomf(float from, float to) {
	return (random() / 2147483647.0f) * (to - from) + from;
}

static
size_t round_to_next_multiple_of(size_t qty, unsigned alignment) {
	return (qty + alignment - 1) / alignment * alignment;
}

static
void emitter_spawn_at(emitter_t* emitter, size_t i) {
	emitter->particles_pos[i].x = emitter->spawn_pos.x + emitter->spawn_pos_spread.x * randomf(-0.5f, 0.5f);
	emitter->particles_pos[i].y = emitter->spawn_pos.y + emitter->spawn_pos_spread.y * randomf(-0.5f, 0.5f);
	emitter->particles_vel[i].x = emitter->spawn_vel.x + emitter->spawn_vel_spread.x * randomf(-0.5f, 0.5f);
	emitter->particles_vel[i].y = emitter->spawn_vel.y + emitter->spawn_vel_spread.y * randomf(-0.5f, 0.5f);
	emitter->particles_color[i] = emitter->spawn_colors[random() % emitter->spawn_colors_count];
	emitter->particles_size[i] = emitter->spawn_size + emitter->spawn_size_spread * randomf(-0.5f, 0.5f);
}

void emitter_spawn_all(emitter_t* emitter) {
	size_t count = emitter->particles_count;

	for (size_t i = 0; i < count; i++) {
		emitter_spawn_at(emitter, i);
	}
}
void emitter_clear_all(emitter_t* emitter) {
	for (size_t i = 0; i < emitter->particles_count; i++) {
		emitter->particles_color[i].a = 0;
	}
}

void emitter_respawn(emitter_t* emitter, size_t spawn_limit) {
	size_t count = emitter->particles_count;
	size_t spawn_count = 0;

	for (size_t i = 0; i < count; i++) {
		if (spawn_count >= spawn_limit) break;
		if (
			emitter->particles_color[i].a
			&& iabs(emitter->particles_pos[i].y) < 1000
			&& iabs(emitter->particles_pos[i].x) < 1000
			&& 0.5 < emitter->particles_size[i]
		) continue;
		emitter_spawn_at(emitter, i);
		spawn_count++;
	}
}

emitter_t* emitter_create(size_t max_particles) {
	static const SDL_Color no_colors[1] = { { 0, 0, 0, 0 } };
	const size_t capacity = round_to_next_multiple_of(max_particles, 64);

	emitter_t* emitter = malloc(sizeof(emitter_t));
	*emitter = (emitter_t) {
		// Spawn config
		.spawn_pos = (vec2_t) { 0, 0 },
		.spawn_pos_spread = (vec2_t) { 0, 0 },
		.spawn_vel = (vec2_t) { 0, 0 },
		.spawn_vel_spread = (vec2_t) { 0, 0 },
		.spawn_size = 0,
		.spawn_size_spread = 0,
		.spawn_colors_count = 1,
		.spawn_colors = no_colors,
		// Particles
		.particles_count = max_particles,
		.particles_capacity = max_particles,
		.particles_pos = malloc(sizeof(vec2_t[capacity])),
		// FIXME: El ownership de `particles_color` está cursed
		.particles_color = malloc(sizeof(SDL_Color[capacity])),
		.particles_size = malloc(sizeof(float[capacity])),
		.particles_vel = malloc(sizeof(vec2_t[capacity])),
	};
	return emitter;
}

void emitter_destroy(emitter_t* emitter) {
	free(emitter->particles_pos);
	free(emitter->particles_color);
	free(emitter->particles_size);
	free(emitter->particles_vel);
	free(emitter);
}
