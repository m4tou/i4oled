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
#include <math.h>
#include <locale.h>
#include <png.h>
#include <pango/pangocairo.h>
#include <pango/pango.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>

#define VER "0.3"
#define SIZE 40
#define MAX_LEN 11

void split_text(wchar_t *source, char* line1, char* line2) 
{
	wchar_t buf[SIZE];
	wchar_t delimiters[SIZE] = L" -+_";
	wchar_t wcsline1[SIZE] = L"";
	wchar_t wcsline2[SIZE] = L"";
	wchar_t* token;
	wchar_t* state;
	int i;
	int token_len[SIZE >> 1]; /*Maximum number of tokens equals half of maximum number of characters*/
	size_t length, l;

	wcscpy(buf, source);
	token = wcstok(buf, delimiters, &state);

	if (wcslen(token) > MAX_LEN) {
		wcsncpy(wcsline1, source, MAX_LEN);
		wcsncpy(wcsline2, source + MAX_LEN, SIZE - MAX_LEN);
		goto out;
	}

	for (i = 0; i < 10; i++)
		token_len[i] = 0;

	i = 0;
	while (token) {
		token_len[i] = wcslen(token) + 1;
		i++;
		token = wcstok(NULL, delimiters, &state);
	}

	i = 0;
	length = token_len[i];
	while ((length + token_len[i + 1]) <= MAX_LEN) {  
		i++;
		length = length + token_len[i];
	}	

	wcsncpy(wcsline1, source, length - 1);
	wcsncpy(wcsline2, source + length, SIZE - length);
out:
	l = wcstombs(line1, wcsline1, MAX_LEN);
	if (l == -1) { 
		printf("Invalid character sequance - please try a different text");
	}	

	l = wcstombs(line2, wcsline2, SIZE - MAX_LEN);
	if (l == -1) { 
		printf("Invalid character sequance - please try a different text");
	}	
}

int rendertext(wchar_t* text, char* output_filename) {
	cairo_t *cr;
	cairo_status_t status;
	cairo_surface_t *surface;
	PangoFontDescription *desc;
	PangoLayout *layout;
	int width, height;
	double x, y;
	char line1[SIZE] = "";
	char line2[SIZE] = "";
	char buf[SIZE];

	split_text(text ,line1, line2);
	strcpy(buf, line1);
	strcat(buf, "\n");
	strcat(buf, line2);

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 64, 32);
	cr = cairo_create(surface);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.99);
	cairo_paint(cr);

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
	pango_layout_set_text(layout, buf, -1);
	desc = pango_font_description_new();

	pango_font_description_set_family(desc, "Terminal");
	pango_font_description_set_absolute_size(desc, PANGO_SCALE * 11);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	pango_layout_get_size(layout, &width, &height);
	width = width/PANGO_SCALE;
	cairo_new_path(cr);

	x = trunc((64.0 - width)/2);

	if (!strcmp(line2, ""))
		y = 10;
	else  
		y = 4;
	

	cairo_move_to(cr, x, y);
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	pango_cairo_update_layout(cr, layout);

	pango_cairo_layout_path(cr, layout);
	cairo_fill(cr);

	g_object_unref(layout);
	cairo_destroy(cr);

	if (output_filename) {
		status = cairo_surface_write_to_png(surface, output_filename);
		if (status != CAIRO_STATUS_SUCCESS) {
			printf("Could not save to png, \"%s \"\n", output_filename);
			return 1;
		}
	}
	cairo_surface_destroy(surface);
	return 0;
}

static int wacom_read_image(const char *filename, unsigned char image[1024])
{
	unsigned char header[8];
	unsigned char lo, hi;
	int x, y;
	int ret = 0;
	int width, height;
	png_byte color_type;
	png_byte bit_depth;
	png_structp png_ptr;
	png_infop info_ptr;
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
	int ret;

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
	"\n"
	"i4oled sets OLED icon on Intuos4 tablets. Also converts text to png image ready for use as Intuos4 OLED icon.\n"
	"Usage: i4oled [options] [device image]\n"
	"Options:\n"
	" -h, --help                 - usage\n"
	" -d, --device               - path to OLED sysfs entry\n"
	" -o, --output         	     - output png file\n"
	" -s, --scramble             - scramble image before sending. Useful for kernel without the 'scramble' patch\n"
	" -t, --text         	     - text string for convertsion into image\n"
	" -V, --version              - version info\n");

	printf(
	"Usage:\n"
	"i4oled --text \"Ctrl+Alt A\" --output ctrl_alt_A.png renders text to PNG image\n"
	"i4oled --image ctrl_alt_A --device  /sys/bus/usb/drivers/wacom/3-1.2:1.0/wacom_led/button0_rawimg\n"
	"Make sure OLED brightness is set, otherwise icons will be black\n"
	"echo 200 > /sys/bus/usb/drivers/wacom/3-1.2:1.0/wacom_led/buttons_luminance\n"
	"Expected image format is:\n"
	"PNG image file, has to be 64 x 32, 8-bit/color RGBA, non-interlaced \n");
}

#define SIZE 40

int main (int argc, char **argv)
{
	int c, ret = 0;
	int optidx;
	int scramble_image;
	char* device_filename = NULL;
	char* image_filename = NULL;
	char* output_filename = NULL;
	char* char_text = NULL;
	wchar_t text[SIZE] = L"";
	unsigned char image[1024];
	size_t length;

	struct option options[] = {
		{"help", 0, NULL, 0},
		{"device", 0, NULL, 0},
		{"image", 0, NULL, 0},
		{"output", 0, NULL, 0},
		{"scramble", 0, NULL, 0},
		{"text", 0, NULL, 0},
		{"version", 0, NULL, 0},
		{NULL, 0, NULL, 0}
	};

	if (!setlocale(LC_CTYPE, "")) {
		fprintf(stderr, "Can't set the specified locale! "
			"Check LANG, LC_CTYPE, LC_ALL.\n");
		return 1;
	}

	scramble_image = 0;

	if (argc < 2)
	{
		usage();
		return 1;
	}

	while ((c = getopt_long(argc, argv, "hd:i:o:st:V", options, &optidx)) != -1) {
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
						output_filename = argv[optind];
						break;
					case 4:
						scramble_image = 1;
						break;
					case 5:
						char_text = argv[optind];
						length = mbstowcs(text, char_text, strlen(char_text));
						if (length == -1) { 
							printf("Invalid character sequance - please try a different text");
							return 1;
						}
						break;
					case 6:
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
			case 'o':
				output_filename = argv[optind-1];
				break;
			case 's':
				scramble_image = 1;
				break;
			case 't':
				char_text = argv[optind-1];
				length = mbstowcs(text, char_text, strlen(char_text));
				if (length == -1) { 
					printf("Invalid character sequance - please try a different text");
					return 1;
				};
				break;
			case 'V':
				version();
				return 0;
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

	if (wcscmp(text, L"")) {
		if (rendertext(text, output_filename))
			goto out;
	}

	if (device_filename){
		ret = wacom_oled_write(device_filename, image);
}

out:
	return ret;
}

