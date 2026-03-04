#include "properties.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "emitter.h"
#include "font.h"

void nop(void* ptr) {
}

void float_inc(void* ptr) {
	float* f = ptr;
	if (0.001 < *f)
		*f *= 1.02;
	else if (0.0 <= *f)
		*f = 0.001;
	else if (-0.001 <= *f)
		*f = 0.0;
	else
		*f *= 0.98;
}

void float_dec(void* ptr) {
	float* f = ptr;
	if (0.001 < *f)
		*f *= 0.98;
	else if (0.0 < *f)
		*f = 0.0;
	else if (-0.001 < *f)
		*f = -0.001;
	else
		*f *= 1.02;
}

void float_zero(void* ptr) {
	float* f = ptr;
	*f = 0.0f;
}

void float_linear_inc(void* ptr) {
	float* f = ptr;
	*f += 1;
}

void float_linear_dec(void* ptr) {
	float* f = ptr;
	*f -= 1;
}

void float_print(void* ptr, char buf[32]) {
	float* f = ptr;
	snprintf(buf, 32, "%f", *f);
}
void float_scan(void* ptr, char buf[32]) {
	float* f = ptr;
	sscanf(buf, "%f", f);
}

void bool_inc(void* ptr) {
	bool* b = ptr;
	*b = true;
}

void bool_dec(void* ptr) {
	bool* b = ptr;
	*b = false;
}

void bool_print(void* ptr, char buf[32]) {
	bool* b = ptr;
	snprintf(buf, 32, "%s", *b ? "yes" : "no");
}

void bool_scan(void* ptr, char buf[32]) {
	bool* b = ptr;
	*b = strcmp(buf, "yes") == 0;
}

void uint8_inc(void* ptr) {
	uint8_t* i = ptr;
	if (*i == 255) return;
	(*i)++;
}

void uint8_dec(void* ptr) {
	uint8_t* i = ptr;
	if (*i == 0) return;
	(*i)--;
}

void uint8_zero(void* ptr) {
	uint8_t* i = ptr;
	*i = 0;
}

void uint8_print(void* ptr, char buf[32]) {
	uint8_t* i = ptr;
	snprintf(buf, 32, "%u", *i);
}

void uint8_scan(void* ptr, char buf[32]) {
	uint8_t* i = ptr;
	unsigned int r;
	sscanf(buf, "%u", &r);
	*i = r & 0xff;
}

void emitter_count_inc(void* ptr) {
	emitter_t* emitter = ptr;
	if (emitter->particles_count == emitter->particles_capacity) return;
	emitter->particles_count++;
}

void emitter_count_dec(void* ptr) {
	emitter_t* emitter = ptr;
	if (emitter->particles_count == 0) return;
	emitter->particles_count--;
}

void emitter_count_print(void* ptr, char buf[32]) {
	emitter_t* emitter = ptr;
	snprintf(buf, 32, "%lu", emitter->particles_count);
}

void emitter_count_scan(void* ptr, char buf[32]) {
	emitter_t* emitter = ptr;
	sscanf(buf, "%lu", &emitter->particles_count);
}

void fquickload(property_t properties[], const char* path) {
	int i = 1;
	property_t* curr = &properties[i];

	FILE *f = fopen(path, "r");
	if (!f) return;
	while (!feof(f)) {
		char name[32];
		char val[32];
		int n = fscanf(f, "%[^:]: %[^\n]\n", name, val);
		if (n != 2) {
			break;
		}
		if (0 == strcmp(curr->name, name)) {
			curr->scan(curr->value, val);
		} else {
			break;
		}
		curr++;
	}
	fclose(f);
}

void fquicksave(property_t properties[], const char* path) {
	FILE *f = fopen(path, "w");
	if (!f) return;
	int i = 1; // skip begin marker
	property_t* curr = &properties[i];
	while (curr->name) {
		char buf[24];
		curr->print(curr->value, buf);
		fprintf(f, "%s: %s\n", curr->name, buf);
		curr++;
	}
	fclose(f);
}

void quickload(property_t properties[]) {
	fquickload(properties, "props.txt");
}

void quicksave(property_t properties[]) {
	fquicksave(properties, "props.txt");
}


void properties_print(int y, property_t properties[], property_t* selected_property) {
	int i = 1; // skip begin marker
	property_t* curr = &properties[i];
	while (curr->name) {
		size_t name_len = strlen(curr->name);
		if (curr == selected_property)
			font_print(">", 1, y + i);
		font_print(curr->name, 2, y + i);
		font_print(":", 2 + name_len, y + i);

		char buf[32];
		curr->print(curr->value, buf);
		font_print(buf, 4 + name_len, y+i);

		curr++;
		i++;
	}
}

