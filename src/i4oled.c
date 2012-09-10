/*
 * Copyright © 2012 Przemo Firszt
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:
 *      Przemo Firszt (przemo@firszt.eu)
 */

#include <fcntl.h>
#include <getopt.h>
#include <png.h>
#include <pango/pangocairo.h>
#include <stdio.h>

#define VER "0.2"

static int wacom_read_image(const char *filename, unsigned char image[1024])
{
	char header[8];
	unsigned char lo, hi;
	int retval;
	int length = 1024;
	int x, y;
	int ret = 0;
	int width, height;
	png_byte color_type;
	png_byte bit_depth;
	png_structp png_ptr;
	png_infop info_ptr;
	int number_of_passes;
	png_bytep * row_pointers;

	FILE *fd = fopen(filename, "r");
	if (!fd) {
		ret = 1;
		printf("Failed to open filename: %s\n", filename);
		goto out;
	}

        fread(header, 1, 8, fd);
        if (png_sig_cmp(header, 0, 8)) {
		ret = 1;
                printf("File %s is not a PNG file", filename);
		goto out;
	}

        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr) {
		ret = 1;
                printf("png_create_read_struct failed");
		goto out;
	}

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
		ret = 1;
                printf("png_create_info_struct failed");
		goto out;
	}

        if (setjmp(png_jmpbuf(png_ptr))) {
		ret = 1;
                printf("Error during init_io");
		goto out;
	}

        png_init_io(png_ptr, fd);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	if (width != 64 || height !=32) {
		ret = 1;
                printf("Invalid image size: %dx%d, but expecting 64x32\n", width, height);
		goto out;
	}

	if (color_type != 6 || bit_depth !=8) {
		ret = 1;
                printf("Invalid color type or bit depth, please use RGBA 8-bit png\n"
			"Use 'file' command on the icon. Expected result:\n"
			"PNG image data, 64 x 32, 8-bit/color RGBA, non-interlaced\n");
		goto out;
	}

        png_read_update_info(png_ptr, info_ptr);

        if (setjmp(png_jmpbuf(png_ptr))) {
		ret = 1;
                printf("Error reading image");
		goto out;
	}

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (y = 0; y < height; y++)
                row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        png_read_image(png_ptr, row_pointers);
	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);

        for (y = 0; y < height; y++) {
                png_byte* row = row_pointers[y];
                for (x = 0; x < width ; x++) {
                        png_byte* ptr = &(row[x * 8]);
			hi = ptr[0];
			hi = 0xf0 & hi;
			lo = ptr[4] >> 4;
			image[(32 * y) + x] = hi | lo;
                }
        }

out:
	if (fd)
		fclose(fd);
	return ret;
}


static int wacom_oled_write(const char *filename, unsigned char image[1024])
{
	int retval;
	int length = 1024;
	int fd = -1;
	int x, ret;

	fd = open (filename , O_WRONLY);
	if (fd < 0) {
		ret = 1;
		printf("Failed to open filename: %s\n", filename);
		goto out;
	}

	retval = write (fd, image, length);
	if (retval != length) {
		ret = 1;
		printf("Writing to %s failed\n", filename);
		goto out;
	}
out:
	if (fd >= 0)
 		close (fd);
	return ret;
}

static void scramble(unsigned char image[1024])
{
        unsigned char buf[1024];
        int x, y, i;
	unsigned char l1,l2,h1,h2;

        for (i = 0; i < 1024; i++)
                buf[i] = image[i];

        for (y = 0; y < 16; y++) {
                for (x = 0; x < 32; x++) {
			l1 = (0x0F & (buf[31 - x + 64 * y]));
			l2 = (0x0F & (buf[31 - x + 64 * y] >> 4));
			h1 = (0xF0 & (buf[63 - x + 64 * y] << 4));
			h2 = (0xF0 & (buf[63 - x + 64 * y]));

                        image[(2 * x) + (64 * y)] = h1 | l1;
                        image[(2 * x) + 1 + (64 * y)] = h2 | l2;
                }
        }
}

static void version(void)
{
	printf("%s\n", VER);
}

static void usage(void)
{
	printf(
	"i4oled sets OLED icon on Intuos4 tablets\n"
	"Usage: i4oled [options] [device image]\n"
	"Options:\n"
	" -h, --help                 - usage\n"
	" -s, --scramble             - scramble image before sending. Useful for kernel without the 'scramble' patch\n"
	" -V, --version              - version info\n");

	printf(
	"\ndevice - path to OLED sysfs entry (button[No]_rawimg) i.e /sys/bus/usb/drivers/wacom/3-1.2:1.0/wacom_led/button0_rawimg\n"
	"image  - PNG image file, has to be 64 x 32, 8-bit/color RGBA, non-interlaced \n");
}

int main (int argc, char **argv)
{
	int c, ret;
	int optidx;
	int scramble_image;
	char* device_filename;
	char* image_filename;
	unsigned char image[1024];

	struct option options[] = {
		{"help", 0, NULL, 0},
		{"device", 0, NULL, 0},
		{"image", 0, NULL, 0},
		{"scramble", 0, NULL, 0},
		{"version", 0, NULL, 0},
		{NULL, 0, NULL, 0}
	};

	scramble_image = 0;

	if (argc < 2)
	{
		usage();
		return 1;
	}

	while ((c = getopt_long(argc, argv, "+hsV", options, &optidx)) != -1) {
		switch(c)
		{
			case 0:
				switch(optidx)
				{
					case 0:
						usage();
						return 0;
					case 1:
						device_filename = argv[optind];
						break;
					case 2:
						image_filename = argv[optind];
						break;
					case 3:
						scramble_image = 1;
						break;
					case 4:
						version();
						return 0;
				}
				break;
			case 'd':
				device_filename = argv[optind-1];
				break;
			case 'i':
				image_filename = argv[optind-1];
				break;
			case 'V':
				version();
				return 0;
			case 's':
				scramble_image = 1;
				break;
			case 'h':
			default:
				usage();
				return 0;
		}
	}

	if (image_filename)
		if (wacom_read_image(image_filename, image))
			goto out;

	if (scramble_image)
		scramble(image);

	if (device_filename)
		ret = wacom_oled_write(device_filename, image);

out:
	return ret;
}

