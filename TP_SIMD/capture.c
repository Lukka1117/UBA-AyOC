#include "capture.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "properties.h"

bool capturing = false;
char capture_dir[64];
int  capture_session = 0;
size_t screenshot_count = 0;

void capture_toggle(bool want_capture, bool *reset, property_t properties[]) {
	int pid = getpid();
	if (want_capture) {
		capture_session++;
		screenshot_count = 0;
		snprintf(capture_dir, 63, "screenshots/%06d%03d",
				 pid, capture_session);
		char mkdir_cmd[128];
		snprintf(mkdir_cmd, 127, "mkdir -p %s", capture_dir);
		if (system(mkdir_cmd)) {
			printf("[CAPTURE] Falied to execute `%s`\n", mkdir_cmd);
		}
		*reset = true;
	} else {
		char props_path[128];
		snprintf(props_path, 127, "%s_props.txt", capture_dir);
		fquicksave(properties, props_path);

		char convert_cmd[256];
		snprintf(convert_cmd, 255, "ffmpeg -framerate 60 -i %s/%%06d.bmp -vf format=yuv420p %s.mov",
				capture_dir, capture_dir);
		if (system(convert_cmd)) {
			printf("[CAPTURE] Falied to execute `%s`\n", convert_cmd);
		}
	}
	capturing = want_capture;
}

void capture_frame(SDL_Window *window, bool *want_capture) {
	if (capturing) {
		char screen_path[128];
		snprintf(screen_path, 127, "%s/%06ld.bmp",
				 capture_dir, screenshot_count++);
		SDL_Surface *s = SDL_GetWindowSurface(window);
		if (!s) {
			printf("%s\n", SDL_GetError());
			*want_capture = false;
		} else {
			SDL_SaveBMP(s, screen_path);
			printf("screen_path: %s\n", screen_path);
		}
	}
}
