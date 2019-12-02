#ifndef COLOR_H
#define COLOR_H

#include <inttypes.h>

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color_rgb_t;


typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} color_rgba_t;

#endif
