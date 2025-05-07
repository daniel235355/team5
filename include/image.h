#ifndef IMAGE_H
#define IMAGE_H

typedef struct {
    int width;
    int height;
    unsigned char *data;
} Image;

Image load_image(const char *filename);
void save_image(Image image, const char *filename);
void free_image(Image image);
Image convert_to_grayscale(Image color_image);

#endif // IMAGE_H

