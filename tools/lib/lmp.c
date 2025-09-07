/*
 * LMP File Library Implementation for Carnifex
 * 
 * Implements LMP file reading, writing, and manipulation functionality.
 * This is a stub implementation for basic functionality.
 */

#include "lmp.h"
#include <stdlib.h>
#include <string.h>

/* General LMP operations */
lmp_file_t *lmp_load_from_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    
    lmp_file_t *lmp = malloc(sizeof(lmp_file_t));
    if (!lmp) {
        fclose(f);
        return NULL;
    }
    
    /* Read width and height (4 bytes each) */
    if (fread(&lmp->width, sizeof(uint32_t), 1, f) != 1) {
        free(lmp);
        fclose(f);
        return NULL;
    }
    
    if (fread(&lmp->height, sizeof(uint32_t), 1, f) != 1) {
        free(lmp);
        fclose(f);
        return NULL;
    }
    
    /* Calculate data size */
    lmp->data_size = lmp->width * lmp->height;
    lmp->data = malloc(lmp->data_size);
    if (!lmp->data) {
        free(lmp);
        fclose(f);
        return NULL;
    }
    
    /* Read pixel data */
    if (fread(lmp->data, lmp->data_size, 1, f) != 1) {
        free(lmp->data);
        free(lmp);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    return lmp;
}

bool lmp_save_to_file(lmp_file_t *lmp, const char *filename) {
    if (!lmp || !filename) return false;
    
    FILE *f = fopen(filename, "wb");
    if (!f) return false;
    
    bool success = (fwrite(&lmp->width, sizeof(uint32_t), 1, f) == 1) &&
                   (fwrite(&lmp->height, sizeof(uint32_t), 1, f) == 1) &&
                   (fwrite(lmp->data, lmp->data_size, 1, f) == 1);
    
    fclose(f);
    return success;
}

void lmp_free(lmp_file_t *lmp) {
    if (!lmp) return;
    
    if (lmp->data) free(lmp->data);
    free(lmp);
}

/* Conchars operations */
conchars_t *conchars_load_from_file(const char *filename) {
    lmp_file_t *base = lmp_load_from_file(filename);
    if (!base) return NULL;
    
    conchars_t *conchars = malloc(sizeof(conchars_t));
    if (!conchars) {
        lmp_free(base);
        return NULL;
    }
    
    conchars->base = *base;
    free(base);
    
    /* Check if this is a valid conchars file */
    if (conchars->base.width != CONCHARS_WIDTH || 
        conchars->base.height != CONCHARS_HEIGHT) {
        conchars_free(conchars);
        return NULL;
    }
    
    /* Simple custom detection - in a real implementation this would be more sophisticated */
    conchars->is_custom = false;
    
    return conchars;
}

bool conchars_save_to_file(conchars_t *conchars, const char *filename) {
    if (!conchars) return false;
    return lmp_save_to_file(&conchars->base, filename);
}

void conchars_free(conchars_t *conchars) {
    if (!conchars) return;
    lmp_free(&conchars->base);
    free(conchars);
}

bool conchars_get_character(conchars_t *conchars, int char_index, uint8_t *char_data) {
    if (!conchars || !char_data || char_index < 0 || char_index >= CONCHARS_TOTAL_CHARS) {
        return false;
    }
    
    int chars_per_row = CONCHARS_WIDTH / CONCHARS_CHAR_WIDTH;
    int char_x = (char_index % chars_per_row) * CONCHARS_CHAR_WIDTH;
    int char_y = (char_index / chars_per_row) * CONCHARS_CHAR_HEIGHT;
    
    for (int y = 0; y < CONCHARS_CHAR_HEIGHT; y++) {
        for (int x = 0; x < CONCHARS_CHAR_WIDTH; x++) {
            int src_offset = (char_y + y) * CONCHARS_WIDTH + (char_x + x);
            int dst_offset = y * CONCHARS_CHAR_WIDTH + x;
            char_data[dst_offset] = conchars->base.data[src_offset];
        }
    }
    
    return true;
}

/* PCX conversion operations */
bool lmp_to_pcx(lmp_file_t *lmp, const char *pcx_filename) {
    if (!lmp || !pcx_filename) return false;
    
    FILE *f = fopen(pcx_filename, "wb");
    if (!f) return false;
    
    /* PCX header structure */
    typedef struct {
        char signature;           /* 0x0A */
        char version;            /* 5 */
        char encoding;           /* 1 (RLE) */
        char bits_per_pixel;     /* 8 */
        unsigned short xmin, ymin, xmax, ymax;  /* Image bounds */
        unsigned short hdpi, vdpi;              /* DPI (not used) */
        unsigned char colortable[48];           /* Not used for 8-bit */
        char reserved;           /* 0 */
        char color_planes;       /* 1 */
        unsigned short bytes_per_line;          /* Width */
        unsigned short palette_type;            /* 1 */
        char filler[58];         /* Padding */
    } pcx_header_t;
    
    /* Initialize PCX header */
    pcx_header_t header = {0};
    header.signature = 0x0A;
    header.version = 5;
    header.encoding = 1;
    header.bits_per_pixel = 8;
    header.xmin = 0;
    header.ymin = 0;
    header.xmax = lmp->width - 1;
    header.ymax = lmp->height - 1;
    header.hdpi = 72;
    header.vdpi = 72;
    header.reserved = 0;
    header.color_planes = 1;
    header.bytes_per_line = lmp->width;
    header.palette_type = 1;
    
    /* Write PCX header */
    if (fwrite(&header, sizeof(pcx_header_t), 1, f) != 1) {
        fclose(f);
        return false;
    }
    
    /* Write image data with RLE encoding */
    size_t i = 0;
    while (i < lmp->data_size) {
        unsigned char pixel = lmp->data[i];
        unsigned char count = 1;
        
        /* Count consecutive identical pixels (max 63 for RLE) */
        while (count < 63 && i + count < lmp->data_size && lmp->data[i + count] == pixel) {
            count++;
        }
        
        if (count > 1 || pixel >= 0xC0) {
            /* Use RLE encoding */
            unsigned char rle_byte = 0xC0 | count;
            if (fwrite(&rle_byte, 1, 1, f) != 1) {
                fclose(f);
                return false;
            }
        }
        
        /* Write the pixel value */
        if (fwrite(&pixel, 1, 1, f) != 1) {
            fclose(f);
            return false;
        }
        
        i += count;
    }
    
    /* Write default Quake palette (256 colors) */
    unsigned char palette[768];
    for (int i = 0; i < 256; i++) {
        /* Generate a simple grayscale palette */
        palette[i * 3 + 0] = i;     /* Red */
        palette[i * 3 + 1] = i;     /* Green */
        palette[i * 3 + 2] = i;     /* Blue */
    }
    
    if (fwrite(palette, 768, 1, f) != 1) {
        fclose(f);
        return false;
    }
    
    fclose(f);
    return true;
}
