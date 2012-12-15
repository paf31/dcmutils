#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include "dicom.h"

#define IMAGE_GROUP 0x0028
#define PIXEL_DATA_GROUP 0x7FE0

#define ROWS 0x0010
#define COLUMNS 0x0011
#define BITS_ALLOCATED 0x0100
#define BITS_STORED 0x0101
#define PIXEL_DATA 0x0010

char* filename;
unsigned short width, height, bitsAllocated, bitsStored;
unsigned int window, level;

void writePNG(char* filename, int width, int height, png_bytep buffer) {
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	
	// Open file for writing (binary mode)
	fp = fopen(filename, "wb");

	// Initialize write structure
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	// Initialize info structure
	info_ptr = png_create_info_struct(png_ptr);

	// Setup Exception handling
	setjmp(png_jmpbuf(png_ptr))

	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, width, height,
		8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);

	int y;

	for (y = 0; y < height; y++) {
		png_write_row(png_ptr, buffer + y * width);
	}

	// End write
	png_write_end(png_ptr, NULL);

	fclose(fp);

	png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);

	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
}

void applyWindowAndLevel(short *values, png_bytep image, int bitMask, unsigned int startAt, short min, short max, unsigned int size) {
	int i;

	for (i = 0; i < size; i++) {
		short value = values[i];

		if (value > max) {
			image[startAt + i] = 255;
		} else if (value < min) {
			image[startAt + i] = 0;
		} else {
			image[startAt + i] = ((value & bitMask) - min) * 255 / (max - min);
		}
	}
}

void formatPNGData(char* filename, int width, int height, int bitsAllocated, int bitsStored, void *data) { 
	png_bytep buffer = (png_bytep) malloc(width * height * sizeof (png_byte));

	int bitMask = (1 << bitsStored) - 1;

	int min = level - window / 2;
	int max = level + window / 2;

	short *values = (short*) data;

	applyWindowAndLevel(values, buffer, bitMask, 0, min, max, width * height);

	writePNG(filename, width, height, buffer);

	free(buffer);
}

unsigned long formatPNGDataUnknownVL(char* filename, int width, int height, int bitsAllocated, int bitsStored, struct DCMStream stream, FILE *file) {
	unsigned long totalBytes = 0;

	unsigned short buffer[width];
	unsigned long bytesRead;

	int i;

	for (i = 0; i < height; i++) {
		if (stream.read(file, width, &buffer, &bytesRead) == STREAM_OK) {
			// TODO

			totalBytes += bytesRead;
		}
	}

	return totalBytes;
}

void visitElement(struct Tag tag, ValueRepresentation vr, ValueLength vl, void *bytes) {
	switch (tag.Group) {
		case IMAGE_GROUP:
			switch (tag.Element) {
				case ROWS:
					height = *(unsigned short *)bytes;
					printf("Height=%u\n", height);
					break;
				case COLUMNS:
					width = *(unsigned short *)bytes;
					printf("Width=%u\n", width);
					break;
				case BITS_ALLOCATED:
					bitsAllocated = *(unsigned short *)bytes;
					printf("Bits Allocated=%u\n", bitsAllocated);
					break;
				case BITS_STORED:
					bitsStored = *(unsigned short *)bytes;
					printf("Bits Stored=%u\n", bitsStored);
					break;
			}
			break;
		case PIXEL_DATA_GROUP:
			switch (tag.Element) {
				case PIXEL_DATA:
					formatPNGData(filename, width, height, bitsAllocated, bitsStored, bytes);
					break;
			}
			break;
	}
}

unsigned long visitElementUnknownVL(struct Tag tag, ValueRepresentation vr, struct DCMStream stream, FILE *file) {
	unsigned long bytesSkipped;

	if (tag.Group == PIXEL_DATA_GROUP && tag.Element == PIXEL_DATA) {
		bytesSkipped = formatPNGDataUnknownVL(filename, width, height, bitsAllocated, bitsStored, stream, file);
	} else {
		bytesSkipped = skipUnknownVLData(tag, vr, stream, file);
	}

	return bytesSkipped;
}

int main(int argc, char* args[]) {
	FILE *file;

	if (argc < 5) {
		printf("Usage: dcmpng input output window level\n");
		return 1;
	}

	filename = args[2];
	window = atoi(args[3]);
	level = atoi(args[4]);
	
	if ((file = fopen(args[1], "r")) == NULL){
		printf("Cannot open file %s.\n", args[1]);
	} else {
		struct DCMVisitor visitor;

		visitor.visitElement = &visitElement;
		visitor.visitElementUnknownVL = &visitElementUnknownVL;
		visitor.enterSequence = NULL;
		visitor.exitSequence = NULL;
		visitor.nextSequenceItem = NULL;
		
		readDicomFile(file, visitor);
		fclose(file);
	}
	
	return 0;
}