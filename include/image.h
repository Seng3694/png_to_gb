#ifndef IMAGE_H
#define IMAGE_H

#include "color.h"
#include <inttypes.h>

typedef struct {
	int32_t width;
	int32_t height;
	color_rgb_t* data;
} image_t;

image_t* Image_Load(const char* path);
void Image_Destroy(image_t* image);

void Image_GetDistinctColors(const image_t* image, uint32_t* distinctColorCount, color_rgb_t** distinctColors);

#endif
