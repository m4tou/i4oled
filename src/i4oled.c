/*
 * Copyright © 2012-2013 Przemo Firszt
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation.
 * Author makes no representations about the suitability of this software
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
 *	Przemo Firszt (przemo@firszt.eu)
 */

#include <fcntl.h>
#include <getopt.h>
#include <glib.h>
#include <math.h>
#include <locale.h>
#include <png.h>
#include <pango/pangocairo.h>
#include <pango/pango.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#define OLED_WIDTH 64
#define OLED_HEIGHT 32
#define SIZE 30
#define MAGIC_BASE64 "base64:"
#define MAGIC_BASE64_LEN strlen(MAGIC_BASE64)
#define MAX_LINE_LEN 10
#define USB_COLOR_DEPTH 4 /*4 bits per pixel over USB */
#define BT_COLOR_DEPTH 1 /*1 bit per pixel over bluetootb */
#define USB_IMAGE_LEN OLED_WIDTH * OLED_HEIGHT * USB_COLOR_DEPTH / 8 /*OLED image size in bytes - USB mode*/
#define BT_IMAGE_LEN OLED_WIDTH * OLED_HEIGHT * BT_COLOR_DEPTH / 8 /*OLED image size in bytes - BT mode*/

struct params_s {
	char *device_filename;
	int bt_flag; /* 0 - default USB, 1 - bluetooth, 2 - bluetooth with scramble */
	char *image_filename;
	char *output_filename;
	char *input_base64;
	unsigned char *image;
	wchar_t text[SIZE+1];
};

void i4oled_generate_base64(struct params_s *params)
{
	char *base_string, *base64;
	
	base_string = g_base64_encode (params->image, USB_IMAGE_LEN);
	base64 = g_strconcat (MAGIC_BASE64, base_string, NULL);
	free (base_string);
	wprintf(L"%s\n", base64);
	free (base64);
}

void i4oled_render_base64(struct params_s *params)
{
	char *base_string;
	gsize length;

	base_string = g_strdup (params->input_base64 + MAGIC_BASE64_LEN);
	params->image = g_base64_decode ((const char*)base_string, &length);
	free (base_string);
}

void i4oled_split_text(wchar_t *source, char *line1, char *line2)
{
	wchar_t buf[SIZE+1] = L"";
	wchar_t delimiters[SIZE+1] = L" -+_";
	wchar_t wcsline1[SIZE+1] = L"";
	wchar_t wcsline2[SIZE+1] = L"";
	wchar_t *token;
	wchar_t *state;
	int i;
	int token_len[SIZE >> 1]; /*Max number of tokens is half of max number of characters*/
	size_t length, l;

	wcscpy(buf, source);
	if (wcslen(buf) <= MAX_LINE_LEN) {
		wcsncpy(wcsline1, source, MAX_LINE_LEN);
		goto out;
	}

	token = wcstok(buf, delimiters, &state);

	if (wcslen(token) > MAX_LINE_LEN) {
		wcsncpy(wcsline1, source, MAX_LINE_LEN);
		wcsncpy(wcsline2, source + MAX_LINE_LEN, SIZE - MAX_LINE_LEN);
		goto out;
	}

	for (i = 0; i < (SIZE >> 1); i++)
		token_len[i] = 0;

	i = 0;
	while (token) {
		token_len[i] = wcslen(token);
		i++;
		token = wcstok(NULL, delimiters, &state);
	}

	i = 0;
	length = token_len[i];
	while ((length + token_len[i + 1] + 1) <= MAX_LINE_LEN) {
		i++;
		length = length + token_len[i] + 1;
	}

	wcsncpy(wcsline1, source, length);
	wcsncpy(wcsline2, source + length + 1, SIZE - length);
out:
	l = wcstombs(line1, wcsline1, MAX_LINE_LEN);
	if (l == -1)
		wprintf(L"Invalid character sequance - please try a different text\n");

	length = wcslen(wcsline2) + 1;
	if (length > 1) {
		l = wcstombs(line2, wcsline2, length);
		if (l == -1)
			wprintf(L"Invalid character sequance - please try a different text\n");
	}
}

static void i4oled_scramble(struct params_s *params)
{
	unsigned char buf[USB_IMAGE_LEN];
	unsigned char l1, l2, h1, h2;
	unsigned short mask;
	unsigned short s1, s2, r, r1, r2;
	int i, w, x, y, z;
	switch (params->bt_flag) {
		case 0: // USB
			for (i = 0; i < USB_IMAGE_LEN; i++)
				buf[i] = params->image[i];

			for (y = 0; y < (OLED_HEIGHT / 2); y++) {
				for (x = 0; x < (OLED_WIDTH / 2); x++) {
					l1 = (0x0F & (buf[OLED_HEIGHT - 1 - x + OLED_WIDTH * y]));
					l2 = (0x0F & (buf[OLED_HEIGHT - 1 - x + OLED_WIDTH * y] >> 4));
					h1 = (0xF0 & (buf[OLED_WIDTH - 1 - x + OLED_WIDTH * y] << 4));
					h2 = (0xF0 & (buf[OLED_WIDTH - 1 - x + OLED_WIDTH * y]));

					params->image[(2 * x) + (OLED_WIDTH * y)] = h1 | l1;
					params->image[(2 * x) + 1 + (OLED_WIDTH * y)] = h2 | l2;
				}
			}
			break;
		case 1: // BT, no scrambling
			break;
		case 2: //BT scrambling
			for (x = 0; x < 32; x++) {
				for (y = 0; y < 8; y++)
					buf[(8 * x) + (7 - y)] = params->image[(8 * x) + y];
			}

			/* Change 76543210 into GECA6420 as required by Intuos4 WL
			 *        HGFEDCBA      HFDB7531
			 */
			for (x = 0; x < 4; x++) {
				for (y = 0; y < 4; y++) {
					for (z = 0; z < 8; z++) {
						mask = 0x0001;
						r1 = 0;
						r2 = 0;
						i = (x << 6) + (y << 4) + z;
						s1 = buf[i];
						s2 = buf[i+8];
						for (w = 0; w < 8; w++) {
							r1 |= (s1 & mask);
							r2 |= (s2 & mask);
							s1 <<= 1;
							s2 <<= 1;
							mask <<= 2;
						}
						r = r1 | (r2 << 1);
						i = (x << 6) + (y << 4) + (z << 1);
						params->image[i] = 0xFF & r;
						params->image[i+1] = (0xFF00 & r) >> 8;
					}
				}
			}
			break;
	}
}

void i4oled_text_to_image(struct params_s *params, cairo_surface_t *surface)
{
	unsigned char *csurf;
	int i, x, y;
	unsigned char lo, hi;
	unsigned char b0,b1,b2,b3,b4,b5,b6,b7;

	cairo_surface_flush(surface);
	csurf = cairo_image_surface_get_data(surface);
	i = 0;
	switch (params->bt_flag) {
		case 0: // USB
			for (y = 0; y < OLED_HEIGHT; y++) {
				for (x = 0; x < (OLED_WIDTH >> 1); x++) {
					hi = 0xf0 & csurf[4 * OLED_WIDTH * y + 8 * x + 1];
					lo = 0x0f & (csurf[4 * OLED_WIDTH * y + 8 * x + 5] >> 4);
					params->image[i] = hi | lo;
					i++;
				}
			}
			break;
		case 1: // BT
		case 2: // BT scrambled 
			for (y = 0; y < OLED_HEIGHT; y++) {
				for (x = 0; x < (OLED_WIDTH >> 3); x++) {
					b0 = 0b10000000 & (csurf[4 * OLED_WIDTH * y + 32 * x +  1] >> 0);
					b1 = 0b01000000 & (csurf[4 * OLED_WIDTH * y + 32 * x +  5] >> 1);
					b2 = 0b00100000 & (csurf[4 * OLED_WIDTH * y + 32 * x +  9] >> 2);
					b3 = 0b00010000 & (csurf[4 * OLED_WIDTH * y + 32 * x + 13] >> 3);
					b4 = 0b00001000 & (csurf[4 * OLED_WIDTH * y + 32 * x + 17] >> 4);
					b5 = 0b00000100 & (csurf[4 * OLED_WIDTH * y + 32 * x + 21] >> 5);
					b6 = 0b00000010 & (csurf[4 * OLED_WIDTH * y + 32 * x + 25] >> 6);
					b7 = 0b00000001 & (csurf[4 * OLED_WIDTH * y + 32 * x + 29] >> 7);
					params->image[i] = b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7;
					i++;
				}
			}
			break;
	}
}

int i4oled_render_text(struct params_s *params)
{
	cairo_t *cr;
	cairo_status_t status;
	cairo_surface_t *surface;
	PangoFontDescription *desc;
	PangoLayout *layout;
	int width, height;
	double dx, dy;
	char line1[SIZE+1] = "";
	char line2[SIZE+1] = "";
	char buf[SIZE+1];

	i4oled_split_text(params->text, line1, line2);
	strcpy(buf, line1);
	strcat(buf, "\n");
	strcat(buf, line2);

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, OLED_WIDTH, OLED_HEIGHT);
	cr = cairo_create(surface);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.99);
	cairo_paint(cr);

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
	pango_layout_set_text(layout, buf, -1);
	desc = pango_font_description_new();

	pango_font_description_set_family(desc, "DejaVu");
	pango_font_description_set_absolute_size(desc, PANGO_SCALE * 11);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	pango_layout_get_size(layout, &width, &height);
	width = width/PANGO_SCALE;
	cairo_new_path(cr);

	dx = trunc(((double)OLED_WIDTH - width)/2);

	if (!strcmp(line2, ""))
		dy = 10;
	else
		dy = 4;

	cairo_move_to(cr, dx, dy);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	pango_cairo_update_layout(cr, layout);

	pango_cairo_layout_path(cr, layout);
	cairo_fill(cr);

	i4oled_text_to_image(params, surface);

	g_object_unref(layout);
	cairo_destroy(cr);

	if (params->output_filename) {
		status = cairo_surface_write_to_png(surface, params->output_filename);
		if (status != CAIRO_STATUS_SUCCESS) {
			wprintf(L"Could not save to png, \"%s \"\n", params->output_filename);
			return 1;
		}
	}

	cairo_surface_destroy(surface);
	return 0;
}

static int i4oled_read_image(struct params_s *params)
{
	unsigned char header[8];
	unsigned char lo, hi;
	int x, y, i;
	int ret = 0;
	int width, height;
	png_byte color_type;
	png_byte bit_depth;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers;
	unsigned char b0,b1,b2,b3,b4,b5,b6,b7;

	FILE *fd = fopen(params->image_filename, "r");
	if (!fd) {
		ret = 1;
		wprintf(L"Failed to open params->filename: %s\n",
				params->image_filename);
		goto out;
	}

	fread(header, 1, 8, fd);
	if (png_sig_cmp(header, 0, 8)) {
		ret = 1;
		wprintf(L"File %s is not a PNG file", params->image_filename);
		goto out;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		ret = 1;
		wprintf(L"png_create_read_struct failed");
		goto out;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		ret = 1;
		wprintf(L"png_create_info_struct failed");
		goto out;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		ret = 1;
		wprintf(L"Error during init_io");
		goto out;
	}

	png_init_io(png_ptr, fd);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	if (width != OLED_WIDTH || height != OLED_HEIGHT) {
		ret = 1;
		wprintf(L"Invalid image size: %dx%d, but expecting 64x32\n", width, height);
		goto out;
	}

	if (color_type != 6 || bit_depth != 8) {
		ret = 1;
		wprintf(L"Invalid color type or bit depth, please use RGBA 8-bit png\n"
			"Use 'file' command on the icon. Expected result:\n"
			"PNG image data, 64 x 32, 8-bit/color RGBA, non-interlaced\n");
		goto out;
	}

	png_read_update_info(png_ptr, info_ptr);

	if (setjmp(png_jmpbuf(png_ptr))) {
		ret = 1;
		wprintf(L"Error reading image");
		goto out;
	}

	row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * height);
	for (y = 0; y < height; y++)
		row_pointers[y] = (png_byte *) malloc(png_get_rowbytes(png_ptr, info_ptr));

	png_read_image(png_ptr, row_pointers);
	png_set_packing(png_ptr);

	i = 0;
	switch (params->bt_flag) {
		case 0: // USB
			for (y = 0; y < height; y++) {
				png_byte *row = row_pointers[y];
				for (x = 0; x < (width >> 1) ; x++) {
					png_byte *ptr = &(row[x * 8]);
					hi = 0xf0 & ptr[0];
					lo = 0x0f & (ptr[4] >> 4);
					params->image[i++] = hi | lo;
				}
			}
			break;
		case 1: // BT, No scramble
		case 2: // BT, scramble
			for (y = 0; y < OLED_HEIGHT; y++) {
				png_byte *row = row_pointers[y];
				for (x = 0; x < (OLED_WIDTH >> 3); x++) {
					png_byte *ptr = &(row[x * 32]);
					b0 = 0b10000000 & (ptr[ 1] >> 0);
					b1 = 0b01000000 & (ptr[ 5] >> 1);
					b2 = 0b00100000 & (ptr[ 9] >> 2);
					b3 = 0b00010000 & (ptr[13] >> 3);
					b4 = 0b00001000 & (ptr[17] >> 4);
					b5 = 0b00000100 & (ptr[21] >> 5);
					b6 = 0b00000010 & (ptr[25] >> 6);
					b7 = 0b00000001 & (ptr[29] >> 7);
					params->image[i] = b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7;
					i++;
				}
			}
			break;
	}

out:
	if (fd)
		fclose(fd);
	return ret;
}

static int i4oled_oled_write(struct params_s *params)
{
	int retval;
	int length;
	int fd = -1;
	int ret = 0;

	if (!params->bt_flag)
		 length = USB_IMAGE_LEN;
	else
		 length = BT_IMAGE_LEN;

	fd = open(params->device_filename, O_WRONLY);
	if (fd < 0) {
		ret = 1;
		wprintf(L"Failed to open filename: %s\n", params->device_filename);
		goto out;
	}

	retval = write(fd, params->image, length);
	if (retval != length) {
		ret = 1;
		wprintf(L"Writing to %s failed\n", params->device_filename);
		goto out;
	}
out:
	if (fd >= 0)
		close(fd);
	return ret;
}

static void i4oled_version(void)
{
	wprintf(L"%s\n", PACKAGE_VERSION);
}

static void i4oled_usage(void)
{
	wprintf(
	L"i4oled sets OLED icon on Intuos4 tablets. Also converts text to png image ready for use as Intuos4 OLED icon.\n"
	L"Usage: i4oled [options] [device image]\n"
	L"Options:\n\n"
	L" -h, --help			- usage\n"
	L" -B, --scrambled_bluetooth	- bluetooth mode, OLED image will be 1-bit and scrambled\n"
	L" -b, --bluetooth		- bluetooth mode, OLED image will be 1-bit\n"
	L" -d, --device			- path to OLED sysfs entry [o]\n"
	L" -i, --image			- image to be rendered on OLED display [i]\n"
	L" -a, --ibase64			- base64 encoded string as input [i]\n"
	L" -o, --output			- output png file [o]\n"
	L" -s, --obase64			- output to base64 encoded string [o]\n"
	L" -t, --text			- text string for convertsion into image [i]\n"
	L" -V, --version			- version info\n\n"
	L"[i] denotes input, [o] 	- output. Exactly one input and at least one output must be present\n");

	wprintf(
	L"Typical usage:\n"
	L"\n"
	L"Render text to PNG image\n"
	L"i4oled --text \"Ctrl+Alt A\" --output ctrl_alt_A.png\n"
	L"\n"
	L"Render image to OLED display\n"
	L"i4oled --image ctrl_alt_A.png --device  /sys/bus/usb/drivers/wacom/3-1.2:1.0/wacom_led/button0_rawimg\n"
	L"\n"
	L"Render text directly to OLED display over bluetooth\n"
	L"i4oled -d /sys/bus/hid/drivers/wacom/0005:056A:00BD.0001/oled0_img -t \"Alt+Ctrl+Enter\" --bluetooth\n"
	L"\n"
	L"Convert MY TEXT into image encoded with base64\n"
	L"i4oled -t \"MY TEXT\" -s\n"
	L"\n"
	L"Make sure OLED brightness is set, otherwise icons will be black:\n"
	L"USB:\n"
	L"echo 200 > /sys/bus/usb/drivers/wacom/3-1.2:1.0/wacom_led/buttons_luminance\n"
	L"Bluetooth:\n"
	L"echo 120 > /sys/class/leds/0005:056A:00BD.0001:selector:0/brightness\n"
	L"\n"
	L"Expected image format is:\n"
	L"PNG image file, has to be 64 x 32, 8-bit/color RGBA, non-interlaced\n");
}

int i4oled_acquire_text(struct params_s *params, char *char_text)
{
	int l, length;
	l = strlen(char_text);

	if (l > SIZE) {
		wprintf(L"Text too long: %d characters, but maximum accepted length is %d\n", l, SIZE);
		return 1;
	}

	length = mbstowcs(params->text, char_text, l + 1);

	if (length == -1) {
		wprintf(L"Invalid character sequance - please try a different text\n");
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int c, ret = 0;
	int input_present = 0;
	int output_present = 0;
	int base64_present = 0;
	int device_present = 0;
	struct params_s params;
	struct option options[] = {
		{"scrambled_bluetooth", 0, NULL, 'B'},
		{"bluetooth", 0, NULL, 'b'},
		{"help", 0, NULL, 'h'},
		{"device", 1, NULL, 'd'},
		{"image", 1, NULL, 'i'},
		{"ibase64", 1, NULL, 'a'},
		{"output", 1, NULL, 'o'},
		{"obase64", 0, NULL, 's'},
		{"text", 1, NULL, 't'},
		{"version", 0, NULL, 'V'},
		{NULL, 0, NULL, 0}
	};

	if (!setlocale(LC_CTYPE, "")) {
		fprintf(stderr, "Can't set the specified locale! "
			"Check LANG, LC_CTYPE, LC_ALL.\n");
		return 1;
	}

	params.device_filename = NULL;
	params.bt_flag = 0;
	params.image_filename = NULL;
	params.input_base64 = NULL;
	params.output_filename = NULL;
	params.image = malloc(USB_IMAGE_LEN);
	params.text[0] = (wchar_t)0x0;

	if (argc < 2) {
		i4oled_usage();
		ret = 1;
		goto out;
	}

	while ((c = getopt_long(argc, argv, "Bbhd:i:a:o:st:V", options, NULL)) != -1) {
		switch (c) {
		case 'B':
			params.bt_flag = 2;
			break;
		case 'b':
			params.bt_flag = 1;
			break;
		case 'd':
			params.device_filename = argv[optind-1];
			device_present++;
			output_present++;
			break;
		case 'i':
			params.image_filename = argv[optind-1];
			input_present++;
			break;
		case 'a':
			params.input_base64 = argv[optind-1];
			input_present++;
			break;
		case 'o':
			params.output_filename = argv[optind-1];
			output_present++;
			break;
		case 's':
			base64_present++;
			output_present++;
			break;
		case 't':
			if (i4oled_acquire_text(&params, argv[optind-1])) {
				ret = 1;
				goto out;
			}
			input_present++;
			break;
		case 'V':
			i4oled_version();
			ret = 0;
			goto out;
		case 'h':
		default:
			i4oled_usage();
			ret = 0;
			goto out;
		}
	}

	if (input_present != 1) {
		wprintf(L"Please use --text OR --image, multiple input option are not allowed\n");
		ret = 1;
		goto out;
	}

	if ((!output_present) && (!device_present)) {
		wprintf(L"Please make sure that there is an output specified with --device, --output or --obase64\n");
		ret = 1;
		goto out;
	}

	if (device_present > 1) {
		wprintf(L"Multiple --device options are not allowed\n");
		ret = 1;
		goto out;
	}

	if (!output_present) {
		wprintf(L"At least one output must be secified\n");
		ret = 1;
		goto out;
	}

	if (params.input_base64) {
		i4oled_render_base64(&params);
	}

	if (params.image_filename)
		if (i4oled_read_image(&params))
			goto out;

	if (wcscmp(params.text, L"")) {
		if (i4oled_render_text(&params)) {
			ret = 1;
			goto out;
		}
	}

	if (base64_present) {
		i4oled_generate_base64(&params);
	}

	i4oled_scramble(&params);

	if (params.device_filename) {
		ret = i4oled_oled_write(&params);
	}
out:
	free(params.image);
	return ret;
}

