/*
 * PAK File Library for Carnifex
 * 
 * Provides functionality for reading, writing, and manipulating PAK files
 * used by the Quake engine and Carnifex.
 */

#ifndef PAK_H
#define PAK_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* PAK file format constants */
#define PAK_HEADER_ID "PACK"
#define PAK_HEADER_SIZE 12
#define PAK_DIRECTORY_ENTRY_SIZE 64
#define PAK_MAX_FILENAME_LENGTH 56
#define PAK_MAX_FILES 2048

/* PAK file structures */
typedef struct {
    char name[PAK_MAX_FILENAME_LENGTH];
    int32_t filepos;
    int32_t filelen;
} pak_directory_entry_t;

typedef struct {
    char id[4];              /* "PACK" */
    int32_t dirofs;          /* Directory offset */
    int32_t dirlen;          /* Directory length */
} pak_header_t;

typedef struct {
    char *filename;
    int32_t filepos;
    int32_t filelen;
    uint8_t *data;           /* File data (NULL if not loaded) */
} pak_file_t;

typedef struct {
    char *filename;          /* PAK file path */
    pak_header_t header;
    int numfiles;
    pak_file_t *files;
    FILE *handle;            /* File handle for reading */
} pak_t;

/* Function prototypes */

/* PAK file operations */
pak_t *pak_load_from_file(const char *filename);
bool pak_save_to_file(pak_t *pak, const char *filename);
void pak_free(pak_t *pak);

/* PAK file information */
bool pak_is_valid(const char *filename);
int pak_get_file_count(pak_t *pak);
bool pak_file_exists(pak_t *pak, const char *filename);

/* File operations within PAK */
pak_file_t *pak_get_file(pak_t *pak, const char *filename);
bool pak_load_file_data(pak_t *pak, const char *filename);
bool pak_extract_file(pak_t *pak, const char *filename, const char *output_path);
bool pak_extract_all(pak_t *pak, const char *output_dir);

/* PAK creation */
pak_t *pak_create_new(void);
bool pak_add_file(pak_t *pak, const char *filename, const char *pak_path);
bool pak_add_file_data(pak_t *pak, const char *pak_path, const uint8_t *data, int32_t size);
bool pak_remove_file(pak_t *pak, const char *filename);

/* Utility functions */
void pak_print_info(pak_t *pak);
void pak_list_files(pak_t *pak);
int32_t pak_calculate_size(pak_t *pak);

/* Endianness conversion */
int32_t pak_little_endian_int32(int32_t value);
uint32_t pak_little_endian_uint32(uint32_t value);

#endif /* PAK_H */
