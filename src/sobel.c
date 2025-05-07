#include "sobel.h"
#include "image.h"
#include <math.h>

Image apply_sobel_operator(const Image *image)
{
    // Create a new image to store the result
    Image result = {
        .width = image->width,
        .height = image->height,
        .channels = 1,
        .data = malloc(image->width * image->height * sizeof(uint8_t))
    };

    // Sobel operator kernels
    int sobel_x[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    int sobel_y[3][3] = {
        {-1, -2, -1},
        {0, 0, 0},
        {1, 2, 1}
    };

    // Apply the Sobel operator
    for (int y = 1; y < image->height - 1; y++)
    {
        for (int x = 1; x < image->width - 1; x++)
        {
            // Get the pixel values
            int pixel_value = image->data[y * image->width + x];

            // Apply the Sobel operator
            int gx = 0;
            int gy = 0;

            // Apply the Sobel operator to the pixel
            for (int ky = 0; ky < 3; ky++)
            {
                for (int kx = 0; kx < 3; kx++)
                {
                    int pixel_x = x + kx - 1;
                    int pixel_y = y + ky - 1;

                    // Get the pixel values
                    int pixel_value = image->data[pixel_y * image->width + pixel_x];

                    // Apply the Sobel operator
                    gx += pixel_value * sobel_x[ky][kx];
                    gy += pixel_value * sobel_y[ky][kx];
                }
            }
            
            // Calculate the gradient magnitude
            int gradient = sqrt(gx * gx + gy * gy);

            // Store the result
            result.data[y * result.width + x] = gradient;
        }
    }

    return result;
}




            
