#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

#include <CLI.h>

#include "color.h"
#include "image.h"

#define RESET_BIT(value, bit) (value = (value & (~(1 << bit))))
#define SET_BIT(value, bit) (value = (value | (1 << bit)))

static void print_help(void);
static bool color_equals(const color_rgb_t left, const color_rgb_t right);
static void cut_into_subimages(
	const uint8_t* image,
	const int32_t imageWidth, const int32_t imageHeight,
	const int32_t subImageWidth, const int32_t subImageHeight,
	uint32_t* subImageCount, uint8_t*** subImages);

//DMG-01 colors
static const color_rgb_t whiteColor =	  { 0x9b, 0xbc, 0x0f }; //00
static const color_rgb_t lightGrayColor = { 0x8b, 0xac, 0x0f }; //01
static const color_rgb_t darkGrayColor =  { 0x30, 0x62, 0x30 }; //10
static const color_rgb_t blackColor =     { 0x0f, 0x38, 0x0f }; //11

int main(int argc, char** argv)
{
	CLI* cli = CLI_Create(3);
	CLI_AddArgument(cli, 'i', ARG_TYPE_REQUIRED);
	CLI_AddArgument(cli, 'm', ARG_TYPE_OPTION);
	CLI_AddArgument(cli, 'h', ARG_TYPE_COMMAND);
	const cli_errors errors = CLI_Parse(cli, argc, argv);

	if (HAS_FLAG(errors, ERROR_NO_ARGS) || CLI_OptionSet(cli, 'h') == CLI_TRUE)
	{
		CLI_Destroy(cli);
		print_help();
		return 0;
	}

	char* filePath = NULL;

	if (HAS_FLAG(errors, ERROR_ARGS_MISSING) && argc == 2)
	{
		filePath = strdup(argv[1]);
	}
	else if (CLI_TryGetArgument(cli, 'i', &filePath) == CLI_FALSE)
	{
		CLI_Destroy(cli);
		free(filePath);
		print_help();
		return 1;
	}

	const int32_t tileWidth = 8;
	const int32_t tileHeight = CLI_OptionSet(cli, 'm') == CLI_TRUE ? 16 : 8;

	image_t* image = Image_Load(filePath);
	if (!image)
	{
		printf("Invalid image \"%s\".", filePath);
		return 1;
	}

	if (image->width < tileWidth
		|| image->height < tileHeight
		|| image->width % tileWidth != 0
		|| image->width % tileHeight != 0)
	{
		printf("Invalid size %dx%d. Image has to be dividable by %dx%d.\n", image->width, image->height, tileWidth, tileHeight);
		return 1;
	}

	uint32_t colorCount = 0;
	color_rgb_t* colors = NULL;
	Image_GetDistinctColors(image, &colorCount, &colors);

	//check if colors are valid
	for (uint32_t i = 0; i < colorCount; ++i)
	{
		bool isValid = color_equals(colors[i], whiteColor)
			|| color_equals(colors[i], lightGrayColor)
			|| color_equals(colors[i], darkGrayColor)
			|| color_equals(colors[i], blackColor);

		if (!isValid)
		{
			printf("Invalid color: r:%3d g:%3d b:%3d\n", colors[i].r, colors[i].g, colors[i].b);
			Image_Destroy(image);
			return 1;
		}
	}

	//convert images to numeric arrays (0123 => white,lightgray,darkgray,black)
	const uint32_t pixelCount = image->width * image->height;
	uint8_t* gbColorValues = malloc(sizeof(uint8_t) * pixelCount);
	for (uint32_t i = 0; i < pixelCount; ++i)
	{
		const color_rgb_t* current = &image->data[i];
		if (color_equals(*current, whiteColor))
			gbColorValues[i] = 0b00;
		else if (color_equals(*current, lightGrayColor))
			gbColorValues[i] = 0b01;
		else if (color_equals(*current, darkGrayColor))
			gbColorValues[i] = 0b10;
		else if (color_equals(*current, blackColor))
			gbColorValues[i] = 0b11;
	}

	//slice image if it's bigger than 8x8 (8x16 if option is enabled)
	uint32_t subImageCount = 0;
	uint8_t** subImages = NULL;
	cut_into_subimages(gbColorValues, image->width, image->height, tileWidth, tileHeight, &subImageCount, &subImages);
	Image_Destroy(image);

	//for each 8 pixels we need 2 bytes =>
	const uint32_t totalByteCount = (pixelCount / 8) * 2;
	const uint32_t subImagePixelCount = tileWidth * tileHeight;
	const uint32_t subImageByteCount = (subImagePixelCount / 8) * 2;
	uint8_t* bytes = malloc(totalByteCount);
	memset(bytes, 0, totalByteCount);

	//convert image into gb format
	//let's assume following image (dots are 0 => white, 3 => black):
	//
	// .3 33 33 ..
	// 33 .. .3 3.
	// 33 .. .3 3.
	// 33 33 33 3.
	// 33 .. .3 3.
	// 33 .. .3 3.
	// 33 .. .3 3.
	// .. .. .. ..
	//
	// let's take the first row:
	// 0 3 3 3 3 3 0 0 => 00 11 11 11 11 11 00 00
	// now these 8 numbers will be split into two bytes
	// the higher bit into the first one, the lower bit into the second one
	//
	// step1: value=00  => b1: 0        b2: 0
	// step2: value=11  => b1: 01       b2: 01
	// step3: value=11  => b1: 011      b2: 011
	// ...
	// step8: value=00  => b1: 01111100 b2: 01111100  => 7c 7c
	// 
	// this is done with each line and the bytes are aligned in memory
	//
	// NOTE: C has a different endianess than the GameBoy, so the bits will be written in reverse order

	for (uint32_t s = 0; s < subImageCount; ++s)
	{
		for (uint32_t i = 0; i < subImagePixelCount; i+=8)
		{
			const uint32_t index = (s * subImageByteCount) + (i / 8) * 2;
			uint8_t* byte1 = &bytes[index];
			uint8_t* byte2 = &bytes[index + 1];

			for (uint32_t pixel = 0; pixel < 8; ++pixel)
			{
				switch (subImages[s][i + pixel])
				{
				case 0b00:
					RESET_BIT(*byte1, 7 - pixel);
					RESET_BIT(*byte2, 7 - pixel);
					break;
				case 0b01:
					SET_BIT(*byte1, 7 - pixel);
					RESET_BIT(*byte2, 7 - pixel);
					break;
				case 0b10:
					RESET_BIT(*byte1, 7 - pixel);
					SET_BIT(*byte2, 7 - pixel);
					break;
				case 0b11:
					SET_BIT(*byte1, 7 - pixel);
					SET_BIT(*byte2, 7 - pixel);
					break;
				}
			}
		}
	}

	//output to console
	*strchr(filePath, '.') = '\0';
	for (uint32_t i = 0; i < subImageCount; ++i)
	{
		const uint32_t offset = subImageByteCount * i;
		if(subImageCount == 1)
			printf("%s: db ", filePath);
		else
			printf("%s_%d: db ", filePath, i);

		for (uint32_t byte = 0; byte < subImageByteCount; ++byte)
		{
			if (byte < subImageByteCount - 1)
				printf("$%02x, ", bytes[offset + byte]);
			else
				printf("$%02x\n", bytes[offset + byte]);
		}
	}
	printf("%s_end:", filePath);

	for (uint32_t i = 0; i < subImageCount; ++i)
		free(subImages[i]);
	free(subImages);
	free(bytes);
	free(filePath);
	CLI_Destroy(cli);

	return 0;
}

static void print_help(void)
{
	puts("usage: png_to_gb [-i ImagePath (first arg default)] [-m 8x16 Mode (8x8 default)]\n");
}

static bool color_equals(const color_rgb_t left, const color_rgb_t right)
{
	return left.r == right.r
		&& left.g == right.g
		&& left.b == right.b;
}

static void cut_into_subimages(
	const uint8_t* image, 
	const int32_t imageWidth, const int32_t imageHeight, 
	const int32_t subImageWidth, const int32_t subImageHeight, 
	uint32_t* subImageCount, uint8_t*** subImages)
{
	const int32_t columns = imageWidth / subImageWidth;
	const int32_t rows = imageHeight / subImageHeight;
	if (columns == 0 || rows == 0) return;

	const uint32_t imageCount = (uint32_t)(columns * rows);
	*subImageCount = imageCount;
	*subImages = malloc(sizeof(uint8_t*) * (imageCount));

	for (uint32_t i = 0; i < imageCount; ++i)
	{
		uint8_t* current = malloc(sizeof(uint8_t) * subImageWidth * subImageHeight);

		const int32_t originY = (i / columns) * subImageHeight;
		const int32_t originX = (i % columns) * subImageWidth;

		for (uint32_t y = 0; y < (uint32_t)subImageHeight; ++y)
		{
			memcpy(
				current + (y * subImageWidth),
				&image[(originY + y) * imageWidth + originX],
				sizeof(uint8_t) * subImageWidth
			);
		}

		(*subImages)[i] = current;
	}
}
