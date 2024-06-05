#include "png_utils.hpp"
#include <cstdio>
#include <cstdlib>

// Helper function to read PNG file, GPT generated
void readPNGFile(const char *file_name, unsigned char **image_data, int *width, int *height, int *channels) {
    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
        perror("Failed to open file");
        return;
    }

    // Check the PNG signature
    png_byte header[8];
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
        fprintf(stderr, "Not a PNG file\n");
        fclose(fp);
        return;
    }

    // Create and initialize the png_struct
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "png_create_read_struct failed\n");
        fclose(fp);
        return;
    }

    // Create and initialize the png_info
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fprintf(stderr, "png_create_info_struct failed\n");
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return;
    }

    // Set error handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error during init_io\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    // Read the PNG file
    png_read_info(png_ptr, info_ptr);

    *width = png_get_image_width(png_ptr, info_ptr);
    *height = png_get_image_height(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);

    // Adjustments based on color type
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);
    
    png_read_update_info(png_ptr, info_ptr);

    *channels = png_get_channels(png_ptr, info_ptr);

    png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    *image_data = (unsigned char*)malloc(rowbytes * (*height) * sizeof(png_byte));

    png_bytep *row_pointers = (png_bytep*)malloc((*height) * sizeof(png_bytep));
    for (int i = 0; i < *height; i++) {
        row_pointers[i] = *image_data + i * rowbytes;
    }

    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, NULL);

    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
}

/**
 * @brief Copilot generated function, be careful, but I am not going to mess with libpng all by myself
 * 
 * @param file_name 
 * @return GLuint 
 */
GLuint loadPNG(const char *file_name,  int* width, int* height) {
    unsigned char *image_data;
    int channels;
    
    readPNGFile(file_name, &image_data, width, height, &channels);
    
    if (!image_data) {
        fprintf(stderr, "Failed to load PNG file\n");
        return 0;
    }

    GLenum format;
    if (channels == 4)
        format = GL_RGBA;
    else if (channels == 3)
        format = GL_RGB;
    else {
        fprintf(stderr, "Unsupported image format\n");
        free(image_data);
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, format, *width, *height, 0, format, GL_UNSIGNED_BYTE, image_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    free(image_data);

    return texture;
}
