/*
 * LMP File Library for Carnifex
 * 
 * Provides functionality for reading, writing, and manipulating LMP files
 * used by the Quake engine and Carnifex.
 */

#ifndef LMP_H
#define LMP_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* LMP file structures */
typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t *data;
    size_t data_size;
} lmp_file_t;

/* Conchars specific structures */
#define CONCHARS_WIDTH 128
#define CONCHARS_HEIGHT 128
#define CONCHARS_CHAR_WIDTH 8
#define CONCHARS_CHAR_HEIGHT 8
#define CONCHARS_TOTAL_CHARS 256

typedef struct {
    lmp_file_t base;
    bool is_custom;
} conchars_t;

/* Function prototypes */

/* General LMP operations */
lmp_file_t *lmp_load_from_file(const char *filename);
bool lmp_save_to_file(lmp_file_t *lmp, const char *filename);
void lmp_free(lmp_file_t *lmp);

/* Conchars operations */
conchars_t *conchars_load_from_file(const char *filename);
bool conchars_save_to_file(conchars_t *conchars, const char *filename);
void conchars_free(conchars_t *conchars);
bool conchars_get_character(conchars_t *conchars, int char_index, uint8_t *char_data);

#endif /* LMP_H */
