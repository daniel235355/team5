#include "image.h"
#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1)
typedef struct {
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

Image load_image(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Failed to open BMP file");
        exit(1);
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    if (fileHeader.bfType != 0x4D42) {
        fprintf(stderr, "Not a BMP file\n");
        fclose(fp);
        exit(1);
    }
    if (infoHeader.biBitCount != 24) {
        fprintf(stderr, "Only 24-bit BMP is supported\n");
        fclose(fp);
        exit(1);
    }

    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;
    int row_padded = (width * 3 + 3) & (~3);

    unsigned char *rgb_data = (unsigned char *)malloc(row_padded * height);
    unsigned char *gray_data = (unsigned char *)malloc(width * height);

    if (!rgb_data || !gray_data) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(fp);
        exit(1);
    }

    fseek(fp, fileHeader.bfOffBits, SEEK_SET);
    fread(rgb_data, row_padded, height, fp);
    fclose(fp);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * row_padded + x * 3;
            unsigned char b = rgb_data[idx];
            unsigned char g = rgb_data[idx + 1];
            unsigned char r = rgb_data[idx + 2];
            unsigned char gray = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
            gray_data[(height - y - 1) * width + x] = gray; // BMP ¬O bottom-up
        }
    }

    free(rgb_data);

    Image img;
    img.width = width;
    img.height = height;
    img.data = gray_data;
    return img;
}

void save_image(Image image, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Failed to write BMP file");
        exit(1);
    }

    int width = image.width;
    int height = image.height;
    int row_padded = (width * 3 + 3) & (~3);
    int filesize = 54 + row_padded * height;

    BITMAPFILEHEADER fileHeader = {0};
    fileHeader.bfType = 0x4D42;
    fileHeader.bfSize = filesize;
    fileHeader.bfOffBits = 54;

    BITMAPINFOHEADER infoHeader = {0};
    infoHeader.biSize = 40;
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = row_padded * height;

    fwrite(&fileHeader, sizeof(fileHeader), 1, fp);
    fwrite(&infoHeader, sizeof(infoHeader), 1, fp);

    unsigned char *row = (unsigned char *)calloc(row_padded, 1);

    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            unsigned char gray = image.data[y * width + x];
            row[x * 3] = gray;     // B
            row[x * 3 + 1] = gray; // G
            row[x * 3 + 2] = gray; // R
        }
        fwrite(row, row_padded, 1, fp);
    }

    free(row);
    fclose(fp);
}

void free_image(Image image) {
    if (image.data)
        free(image.data);
}

Image convert_to_grayscale(Image image) {
    // already grayscale in our load_bmp, just return it
    return image;
}

