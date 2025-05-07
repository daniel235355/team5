// Detect Edges Using the Sobel Operator

#include <stdio.h>
#include <stdlib.h>
#include "image.h"
#include "sobel.h"

int main(int argc, char *argv[])
{
    // Check if the correct number of arguments are provided
    if (argc != 2)
    {
        printf("Usage: %s <image_path>\n", argv[0]);
        return 1;
    }
    
    // Load the image
    const char *image_path = argv[1];
    Image image = load_image(image_path);

    // Apply Sobel operator for edge detection
    Image sobel_image = apply_sobel_operator(&image);

    // Save the resulting image
    const char *output_path = "edges.bmp";

    save_image(sobel_image, output_path);

    // Free the memory allocated for the images
    free_image(image);
    free_image(sobel_image);
    
    return 0;
}


