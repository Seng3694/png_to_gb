#include "image.h"
#include "color.h"

#include <stb_image.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

image_t* Image_Load(const char* path)
{
	const char* extension = strrchr(path, '.');
	if (!extension) return NULL;
	if (strcmp(extension, ".png") != 1 && strcmp(extension, ".jpg") != 1) return NULL;

	image_t* img = malloc(sizeof(image_t));
	if (!img) return NULL;

	int channels = 0;
	uint8_t* data = stbi_load(path, &img->width, &img->height, &channels, 0);
	if (!data)
	{
		free(img);
		return NULL;
	}

	switch (channels)
	{
	case 4:
	{
		const uint32_t pixels = (uint32_t)(img->width * img->height);
		img->data = malloc(sizeof(color_rgb_t) * pixels);
		if (!img->data)
		{
			free(img);
			stbi_image_free(data);
			return NULL;
		}

		for (uint32_t i = 0; i < pixels; ++i)
			memcpy(&img->data[i], &((color_rgba_t*)data)[i], 3);

		stbi_image_free(data);
		break;
	}
	case 3:
		img->data = (color_rgb_t*)data;
		break;
	default:
		free(img);
		stbi_image_free(data);
		return NULL;
	}

	return img;
}

void Image_Destroy(image_t* image)
{
	if (image)
	{
		free(image->data);
		free(image);
		memset(image, 0, sizeof(image_t));
	}
}

void Image_GetDistinctColors(const image_t* image, uint32_t* distinctColorCount, color_rgb_t** distinctColors)
{
	color_rgb_t* colors = (color_rgb_t*)image->data;
	const uint32_t pixelCount = (uint32_t)(image->width * image->height);

	*distinctColorCount = 0;
	uint32_t distinctColorCapacity = 4;
	*distinctColors = calloc(distinctColorCapacity, sizeof(color_rgb_t));

	for (uint32_t i = 0; i < pixelCount; ++i)
	{
		bool addToColors = true;
		const color_rgb_t* currentColor = &colors[i];

		for (uint32_t c = 0; c < *distinctColorCount; ++c)
		{
			const color_rgb_t* currentDistinctColor = &((*distinctColors)[c]);
			if (currentDistinctColor->r == currentColor->r
				&& currentDistinctColor->g == currentColor->g
				&& currentDistinctColor->b == currentColor->b)
				addToColors = false;
		}

		if (addToColors == true)
		{
			if (*distinctColorCount + 1 > distinctColorCapacity)
			{
				const uint32_t newCapacity = distinctColorCapacity * 2;
				color_rgb_t* newArray = calloc(newCapacity, sizeof(color_rgb_t));
				memcpy(newArray, *distinctColors, distinctColorCapacity * sizeof(color_rgb_t));
				distinctColorCapacity = newCapacity;
				free(*distinctColors);
				*distinctColors = newArray;
			}

			(*distinctColors)[*distinctColorCount] = *currentColor;
			(*distinctColorCount)++;
		}
	}
}
