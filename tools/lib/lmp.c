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
