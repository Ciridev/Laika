#include <stdlib.h>
#include <math.h>

#include "Image/bitmap.h"

LKA_API void* load_bitmap_file(char* filepath)
{
	// Checks if the file exists; it should be, but you never know!
	FILE* file = fopen(filepath, "rb");
	if (!file) return NULL;

	// Make sure that the file is not empty!
	fseek(file, 0, SEEK_END);
	if (ftell(file) == 0) return NULL;

	// Rewind file pointer
	fseek(file, 0, SEEK_SET);

	// Initialize the image pointer
	bitmap_t* image = (bitmap_t*)malloc(sizeof(bitmap_t));

	image->header	   = (bitmap_file_header_t*)		malloc(sizeof(bitmap_file_header_t));
	image->information = (bitmap_information_header_t*) malloc(sizeof(bitmap_information_header_t));
	image->color_table = (bitmap_color_table_t*)	    malloc(sizeof(bitmap_color_table_t));
	image->pixel_array = (bitmap_pixel_array_t*)		malloc(sizeof(bitmap_pixel_array_t));

    image->color_table->colors = NULL;
    
	// Load bitmap data
	load_bitmap_file_header(image, file);
	load_bitmap_information_header(image, file);

	if (image->information->colors > 0)
		load_bitmap_color_table(image, file);

	load_bitmap_image_data(image, file);

	// TODO : Read ICC Profiles if V5 header.
	if (image->information->header_size > 108);

	fclose(file);

	return (void*)image;
}

LKA_API void destroy_bitmap_image(bitmap_t* image)
{
    if(image)
    {
        free(image->header);
        free(image->information);
        
        if(image->color_table->colors)
            free(image->color_table->colors);
        
        free(image->color_table);
        free(image->pixel_array->pixels);
        free(image->pixel_array);
		free(image);
    }
}

LKA_API i32_t save_bitmap_file(char* destination, bitmap_t* image)
{
    FILE* file = fopen(destination, "wb+");
    if(!file) return -1;
    
    if(image->information->colors)
    {
        fclose(file);
        return -1;
    }
    
    fwrite(image->header->header_field,		sizeof(byte_t), 2, file);
    fwrite(&image->header->size,			sizeof(i32_t),	1, file);
	fwrite(image->header->reserved1,		sizeof(byte_t), 2, file);
	fwrite(image->header->reserved2,		sizeof(byte_t), 2, file);
	fwrite(&image->header->offset,			sizeof(i32_t),	1, file);

	fwrite(image->information,		   sizeof(byte_t), image->information->header_size, file);
	fwrite(image->pixel_array->pixels, sizeof(byte_t), image->pixel_array->pixel_count, file);

    fclose(file);
    
    return 0;
}

LKA_API bitmap_t* bitmap_invert(bitmap_t* image, i32_t red, i32_t green, i32_t blue)
{
	if (image->information->bpp < 8) return NULL;

	i32_t bytes = image->information->bpp / 8;
	i32_t width = bytes * image->information->width;
	i32_t pb = image->pixel_array->pixel_count / image->information->height;

	i32_t size = image->pixel_array->pixel_count;

	for (i32_t j = 0; j < size; j++)
	{
		if (check_padding_byte(pb, width, j)) continue;

		if (!red && j % bytes == 0) continue;
		if (!green && j % bytes == 1) continue;
		if (!blue && j % bytes == 2) continue;
		if (bytes == 4 && j % bytes == 3) continue;

		image->pixel_array->pixels[j] = (byte_t)(255 - (i32_t)image->pixel_array->pixels[j]);
	}

	return image;
}

LKA_API bitmap_t* bitmap_set(bitmap_t* image, byte_t set_value, i32_t red, i32_t green, i32_t blue)
{
	if (image->information->bpp < 8) return NULL;

	i32_t bytes = image->information->bpp / 8;
	i32_t width = bytes * image->information->width;
	i32_t pb = image->pixel_array->pixel_count / image->information->height;

	i32_t size = image->pixel_array->pixel_count;

	for (i32_t j = 0; j < size; j++)
	{
		if (check_padding_byte(pb, width, j)) continue;

		if (!red && j % bytes == 0) continue;
		if (!green && j % bytes == 1) continue;
		if (!blue && j % bytes == 2) continue;
		if (bytes == 4 && j % bytes == 3) continue;

		image->pixel_array->pixels[j] = set_value;
	}

	return image;
}

LKA_API bitmap_t* bitmap_grayscale(bitmap_t* image)
{
	if (image->information->bpp < 8) return NULL;

	i32_t bytes = image->information->bpp / 8;
	i32_t width = bytes * image->information->width;
	i32_t pb = image->pixel_array->pixel_count / image->information->height;

	i32_t size = image->pixel_array->pixel_count;

	for (i32_t j = 0; j < size; j += bytes)
	{
		if (check_padding_byte(pb, width, j))
		{
			j -= bytes;
			j += pb - width;
			continue;
		}

		byte_t average = (byte_t)((image->pixel_array->pixels[j] + image->pixel_array->pixels[j + 1] + image->pixel_array->pixels[j + 2]) / 3);

		image->pixel_array->pixels[j] = average;
		image->pixel_array->pixels[j + 1] = average;
		image->pixel_array->pixels[j + 2] = average;
	}

	return image;
}

LKA_API bitmap_t* bitmap_bluescale(bitmap_t* image)
{
	if (image->information->bpp < 8) return NULL;

	i32_t bytes = image->information->bpp / 8;
	i32_t width = bytes * image->information->width;
	i32_t pb = image->pixel_array->pixel_count / image->information->height;

	i32_t size = image->pixel_array->pixel_count;

	for (i32_t j = 0; j < size; j += bytes)
	{
		if (check_padding_byte(pb, width, j))
		{
			j -= bytes;
			j += pb - width;
			continue;
		}

		byte_t average = (byte_t)((image->pixel_array->pixels[j] + image->pixel_array->pixels[j + 1] + image->pixel_array->pixels[j + 2]) / 3);

		image->pixel_array->pixels[j] = average;
		image->pixel_array->pixels[j + 1] = 0;
		image->pixel_array->pixels[j + 2] = 0;
	}

	return image;
}

LKA_API bitmap_t* bitmap_greenscale(bitmap_t* image)
{
	if (image->information->bpp < 8) return NULL;

	i32_t bytes = image->information->bpp / 8;
	i32_t width = bytes * image->information->width;
	i32_t pb = image->pixel_array->pixel_count / image->information->height;

	i32_t size = image->pixel_array->pixel_count;

	for (i32_t j = 0; j < size; j += bytes)
	{
		if (check_padding_byte(pb, width, j))
		{
			j -= bytes;
			j += pb - width;
			continue;
		}

		byte_t average = (byte_t)((image->pixel_array->pixels[j] + image->pixel_array->pixels[j + 1] + image->pixel_array->pixels[j + 2]) / 3);

		image->pixel_array->pixels[j] = 0;
		image->pixel_array->pixels[j + 1] = average;
		image->pixel_array->pixels[j + 2] = 0;
	}

	return image;
}

LKA_API bitmap_t* bitmap_redscale(bitmap_t* image)
{
	if (image->information->bpp < 8) return NULL;

	i32_t bytes = image->information->bpp / 8;
	i32_t width = bytes * image->information->width;
	i32_t pb = image->pixel_array->pixel_count / image->information->height;

	i32_t size = image->pixel_array->pixel_count;

	for (i32_t j = 0; j < size; j += bytes)
	{
		if (check_padding_byte(pb, width, j))
		{
			j -= bytes;
			j += pb - width;
			continue;
		}

		byte_t average = (byte_t)((image->pixel_array->pixels[j] + image->pixel_array->pixels[j + 1] + image->pixel_array->pixels[j + 2]) / 3);

		image->pixel_array->pixels[j] = 0;
		image->pixel_array->pixels[j + 1] = 0;
		image->pixel_array->pixels[j + 2] = average;
	}

	return image;
}

LKA_API bitmap_t* bitmap_colorscale(bitmap_t* image, i32_t color_mask)
{
	if (image->information->bpp < 8) return NULL;

	i32_t bytes = image->information->bpp / 8;
	i32_t width = bytes * image->information->width;
	i32_t pb = image->pixel_array->pixel_count / image->information->height;

	i32_t size = image->pixel_array->pixel_count;

	double red_mask	  = EXTRACT_BYTE(color_mask, 3) / 255.0;
	double green_mask = EXTRACT_BYTE(color_mask, 2) / 255.0;
	double blue_mask  = EXTRACT_BYTE(color_mask, 1) / 255.0;

	for (i32_t j = 0; j < size; j += bytes)
	{
		if (check_padding_byte(pb, width, j))
		{
			j -= bytes;
			j += pb - width;
			continue;
		}

		byte_t average = (byte_t)((image->pixel_array->pixels[j] + image->pixel_array->pixels[j + 1] + image->pixel_array->pixels[j + 2]) / 3);

		image->pixel_array->pixels[j]	  = (byte_t)(average * blue_mask);
		image->pixel_array->pixels[j + 1] = (byte_t)(average * green_mask);
		image->pixel_array->pixels[j + 2] = (byte_t)(average * red_mask);
	}

	return image;
}

LKA_API bitmap_t* bitmap_inverted_colorscale(bitmap_t* image, i32_t color_mask)
{
	if (image->information->bpp < 8) return NULL;

	i32_t bytes = image->information->bpp / 8;
	i32_t width = bytes * image->information->width;
	i32_t pb = image->pixel_array->pixel_count / image->information->height;

	i32_t size = image->pixel_array->pixel_count;

	byte_t red_mask		= EXTRACT_BYTE(color_mask, 3);
	byte_t green_mask	= EXTRACT_BYTE(color_mask, 2);
	byte_t blue_mask	= EXTRACT_BYTE(color_mask, 1);

	for (i32_t j = 0; j < size; j += bytes)
	{
		if (check_padding_byte(pb, width, j))
		{
			j -= bytes;
			j += pb - width;
			continue;
		}

		byte_t average = (byte_t)((image->pixel_array->pixels[j] + image->pixel_array->pixels[j + 1] + image->pixel_array->pixels[j + 2]) / 3);

		image->pixel_array->pixels[j]	  = average * blue_mask;
		image->pixel_array->pixels[j + 1] = average * green_mask;
		image->pixel_array->pixels[j + 2] = average * red_mask;
	}

	return image;
}

LKA_API bitmap_t* bitmap_filter(bitmap_t* image, i32_t rgb_mask)
{
	if (image->information->bpp < 8) return NULL;

	i32_t bytes = image->information->bpp / 8;
	i32_t width = bytes * image->information->width;
	i32_t pb = image->pixel_array->pixel_count / image->information->height;

	i32_t size = image->pixel_array->pixel_count;

	double red_mask = EXTRACT_BYTE(rgb_mask, 3) / 255.0;
	double green_mask = EXTRACT_BYTE(rgb_mask, 2) / 255.0;
	double blue_mask = EXTRACT_BYTE(rgb_mask, 1) / 255.0;

	for (i32_t j = 0; j < size; j += bytes)
	{
		if (check_padding_byte(pb, width, j))
		{
			j -= bytes;
			j += pb - width;
			continue;
		}

		byte_t average = (byte_t)((image->pixel_array->pixels[j] + image->pixel_array->pixels[j + 1] + image->pixel_array->pixels[j + 2]) / 3);

		image->pixel_array->pixels[j] *= blue_mask;
		image->pixel_array->pixels[j + 1] *= green_mask;
		image->pixel_array->pixels[j + 2] *= red_mask;
	}

	return image;
}

LKA_API bitmap_t* bitmap_noise(bitmap_t* image, f64_t factor)
{
	if (image->information->bpp < 8) return NULL;

	i32_t bytes = image->information->bpp / 8;
	i32_t width = bytes * image->information->width;
	i32_t height = image->information->height;
	i32_t pb_size = image->pixel_array->pixel_count / height;
	i32_t pb = pb_size - width;

	i32_t size = image->pixel_array->pixel_count;

	byte_t* convulted_pixels = (byte_t*)malloc(size * sizeof(byte_t));
	byte_t* pixels = image->pixel_array->pixels;

	// Corner indexes
	i32_t ur_corner = pb_size * (height - 1) - pb - 2;
	i32_t lr_corner = 2 * pb_size - pb - 2;
	i32_t ul_corner = pb_size * (height - 2) + 1;
	i32_t ll_corner = pb_size + 1;

	// Kernel offsets
	i32_t u = pb_size;
	i32_t d = -pb_size;
	i32_t l = -1;
	i32_t r = 1;
	i32_t ul = pb_size - 1;
	i32_t ur = pb_size + 1;
	i32_t dl = -pb_size - 1;
	i32_t dr = -pb_size + 1;

	for (i32_t j = 0; j < size; j++)
	{
		if (check_padding_byte(pb_size, width, j))
		{
			for (i32_t i = 0; i < pb; i++) convulted_pixels[i + j] = 0;

			j--;
			j += pb;
			continue;
		}

		i32_t column = j % pb_size;
		i32_t row = j / pb_size;

		i32_t convolution_index = j;

		if (row == 0) convolution_index += pb_size;
		if (row == height - 1) convolution_index -= pb_size;

		if (column == 0) convolution_index += r;
		if (column == pb_size - pb - 1) convolution_index += l;

		byte_t convoluted = (byte_t)(factor * (
			((i32_t)pixels[convolution_index + ul] +
			(i32_t)pixels[convolution_index + u] +
			(i32_t)pixels[convolution_index + ur] +
			(i32_t)pixels[convolution_index + l] + 
			(i32_t)pixels[convolution_index] + 
			(i32_t)pixels[convolution_index + r] +
			(i32_t)pixels[convolution_index + dl] +
			(i32_t)pixels[convolution_index + d] + 
			(i32_t)pixels[convolution_index + dr])));

		convulted_pixels[j] = convoluted;
	}

	free(pixels);
	image->pixel_array->pixels = convulted_pixels;

	return image;
}

i32_t check_padding_byte(i32_t pb, i32_t width, i32_t idx)
{
	return pb && idx % pb > width - 1;
}

void load_bitmap_file_header(bitmap_t* image, FILE* file)
{
	fread(image->header->header_field,	sizeof(byte_t), 2, file);
	fread(&image->header->size,			sizeof(i32_t),	1, file);
	fread(image->header->reserved1,		sizeof(byte_t), 2, file);
	fread(image->header->reserved2,		sizeof(byte_t), 2, file);
	fread(&image->header->offset,		sizeof(i32_t),	1, file);
}

void load_bitmap_information_header(bitmap_t* image, FILE* file)
{
	fread(&image->information->header_size,	sizeof(i32_t), 1, file);
	fread(&image->information->width,		sizeof(u32_t), 1, file);
	fread(&image->information->height,		sizeof(u32_t), 1, file);
	fread(&image->information->planes,		sizeof(i16_t), 1, file);
	fread(&image->information->bpp,			sizeof(i16_t), 1, file);
	fread(&image->information->compression,	sizeof(i32_t), 1, file);
	fread(&image->information->image_size,	sizeof(i32_t), 1, file);
	fread(&image->information->x_ppm,		sizeof(i32_t), 1, file);
	fread(&image->information->y_ppm,		sizeof(i32_t), 1, file);
	fread(&image->information->colors,		sizeof(i32_t), 1, file);
	fread(&image->information->color_count,	sizeof(i32_t), 1, file);

	if (image->information->header_size > 40) // from V2 to V5
	{
		fread(&image->information->red_bitmask,			sizeof(i32_t), 1, file);
		fread(&image->information->green_bitmask,		sizeof(i32_t), 1, file);
		fread(&image->information->blue_bitmask,		sizeof(i32_t), 1, file);
	}

	if (image->information->header_size > 52) // from V3 to V5
	{
		fread(&image->information->alpha_bitmask,	sizeof(i32_t), 1, file);
	}

	if (image->information->header_size > 56) // from V4 to V5
	{
		fread(&image->information->color_space_type,		sizeof(i32_t),  1,	file);
		fread( image->information->color_space_endpoints,	sizeof(byte_t), 36, file);
		fread(&image->information->red_gamma,				sizeof(i32_t),	1,	file);
		fread(&image->information->green_gamma,				sizeof(i32_t),	1,	file);
		fread(&image->information->blue_gamma,				sizeof(i32_t),	1,	file);
	}

	if (image->information->header_size > 108) // V5
	{
		fread(&image->information->intent,	 sizeof(i32_t),  1, file);
		fread(&image->information->icc_data, sizeof(i32_t),  1, file);
		fread(&image->information->icc_size, sizeof(i32_t),	 1, file);
		fread( image->information->reserved, sizeof(byte_t), 4, file);
	}
}

void load_bitmap_color_table(bitmap_t* image, FILE* file)
{
}

void load_bitmap_image_data(bitmap_t* image, FILE* file)
{
	i64_t size = ceil(image->information->bpp * image->information->width / 32.0) * 4 * image->information->height;
    
    image->pixel_array->pixel_count = size;
	image->pixel_array->pixels = (byte_t*)malloc(size * sizeof(byte_t));

	fread(image->pixel_array->pixels, sizeof(byte_t), size, file);
}
