/*
 * PAK File Library Implementation for Carnifex
 * 
 * Implements PAK file reading, writing, and manipulation functionality.
 */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "pak.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <endian.h>

/* Helper function to create directory recursively */
static bool create_directory_recursive(const char *path) {
    char *path_copy = strdup(path);
    char *p = path_copy;
    
    /* Skip leading slash */
    if (*p == '/') p++;
    
    while (*p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
                free(path_copy);
                return false;
            }
            *p = '/';
        }
        p++;
    }
    
    /* Create the final directory */
    if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
        free(path_copy);
        return false;
    }
    
    free(path_copy);
    return true;
}

/* Endianness conversion functions */
int32_t pak_little_endian_int32(int32_t value) {
    /* Convert from little-endian to host byte order */
    return le32toh(value);
}

uint32_t pak_little_endian_uint32(uint32_t value) {
    /* Convert from little-endian to host byte order */
    return ((value & 0xFF) << 24) |
           (((value >> 8) & 0xFF) << 16) |
           (((value >> 16) & 0xFF) << 8) |
           ((value >> 24) & 0xFF);
}

/* Convert from host byte order to little-endian */
int32_t pak_host_to_little_endian_int32(int32_t value) {
    /* Convert from host byte order to little-endian */
    return htole32(value);
}

/* Check if a file is a valid PAK file */
bool pak_is_valid(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return false;
    
    pak_header_t header;
    if (fread(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return false;
    }
    
    fclose(f);
    
    return (memcmp(header.id, PAK_HEADER_ID, 4) == 0);
}

/* Load PAK file from disk */
pak_t *pak_load_from_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open PAK file '%s'\n", filename);
        return NULL;
    }
    
    pak_t *pak = malloc(sizeof(pak_t));
    if (!pak) {
        fclose(f);
        return NULL;
    }
    
    pak->filename = strdup(filename);
    pak->files = NULL;
    pak->handle = f;
    
    /* Read header */
    if (fread(&pak->header, sizeof(pak_header_t), 1, f) != 1) {
        fprintf(stderr, "Error: Cannot read PAK header\n");
        pak_free(pak);
        return NULL;
    }
    
    /* Validate header */
    if (memcmp(pak->header.id, PAK_HEADER_ID, 4) != 0) {
        fprintf(stderr, "Error: Invalid PAK file (bad header ID)\n");
        pak_free(pak);
        return NULL;
    }
    
    /* Auto-detect endianness by checking if values make sense */
    int32_t dirofs_le = le32toh(pak->header.dirofs);
    int32_t dirlen_le = le32toh(pak->header.dirlen);
    int32_t dirofs_be = be32toh(pak->header.dirofs);
    int32_t dirlen_be = be32toh(pak->header.dirlen);
    
    /* Check file size to determine which endianness makes sense */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, PAK_HEADER_SIZE, SEEK_SET);
    
    /* Use little-endian if it makes sense, otherwise use big-endian */
    bool use_little_endian = (dirofs_le > 0 && dirlen_le > 0 && dirofs_le + dirlen_le <= file_size);
    
    if (use_little_endian) {
        pak->header.dirofs = dirofs_le;
        pak->header.dirlen = dirlen_le;
    } else {
        pak->header.dirofs = dirofs_be;
        pak->header.dirlen = dirlen_be;
    }
    
    /* Validate directory */
    if (pak->header.dirlen < 0 || pak->header.dirofs < 0) {
        fprintf(stderr, "Error: Invalid PAK directory (dirofs: %d, dirlen: %d)\n",
                pak->header.dirofs, pak->header.dirlen);
        pak_free(pak);
        return NULL;
    }
    
    pak->numfiles = pak->header.dirlen / PAK_DIRECTORY_ENTRY_SIZE;
    if (pak->numfiles == 0) {
        fprintf(stderr, "Warning: PAK file has no files\n");
        pak_free(pak);
        return NULL;
    }
    
    if (pak->numfiles > PAK_MAX_FILES) {
        fprintf(stderr, "Error: Too many files in PAK (%d > %d)\n", 
                pak->numfiles, PAK_MAX_FILES);
        pak_free(pak);
        return NULL;
    }
    
    /* Allocate file array */
    pak->files = malloc(pak->numfiles * sizeof(pak_file_t));
    if (!pak->files) {
        fprintf(stderr, "Error: Cannot allocate memory for file list\n");
        pak_free(pak);
        return NULL;
    }
    
    /* Seek to directory */
    if (fseek(f, pak->header.dirofs, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Cannot seek to PAK directory\n");
        fclose(f);
        pak->handle = NULL;
        pak_free(pak);
        return NULL;
    }
    
    /* Read directory entries */
    for (int i = 0; i < pak->numfiles; i++) {
        pak_directory_entry_t entry;
        if (fread(&entry, sizeof(pak_directory_entry_t), 1, f) != 1) {
            fprintf(stderr, "Error: Cannot read directory entry %d\n", i);
            fclose(f);
            pak->handle = NULL;
            pak_free(pak);
            return NULL;
        }
        
        /* Use the same endianness as detected for the header */
        if (use_little_endian) {
            entry.filepos = le32toh(entry.filepos);
            entry.filelen = le32toh(entry.filelen);
        } else {
            entry.filepos = be32toh(entry.filepos);
            entry.filelen = be32toh(entry.filelen);
        }
        
        /* Store file info */
        pak->files[i].filename = malloc(PAK_MAX_FILENAME_LENGTH + 1);
        strncpy(pak->files[i].filename, entry.name, PAK_MAX_FILENAME_LENGTH);
        pak->files[i].filename[PAK_MAX_FILENAME_LENGTH] = '\0';
        pak->files[i].filepos = entry.filepos;
        pak->files[i].filelen = entry.filelen;
        pak->files[i].data = NULL;
    }
    
    return pak;
}

/* Save PAK file to disk */
bool pak_save_to_file(pak_t *pak, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot create PAK file '%s'\n", filename);
        return false;
    }
    
    /* Calculate directory offset (after header and all file data) */
    int32_t dirofs = PAK_HEADER_SIZE;
    for (int i = 0; i < pak->numfiles; i++) {
        dirofs += pak->files[i].filelen;
    }
    
    /* Write header */
    pak_header_t header;
    memcpy(header.id, PAK_HEADER_ID, 4);
    header.dirofs = htole32(dirofs);  /* Store in little-endian format (standard Quake PAK) */
    header.dirlen = htole32(pak->numfiles * PAK_DIRECTORY_ENTRY_SIZE);  /* Store in little-endian format (standard Quake PAK) */
    
    if (fwrite(&header, sizeof(pak_header_t), 1, f) != 1) {
        fprintf(stderr, "Error: Cannot write PAK header\n");
        fclose(f);
        return false;
    }
    
    /* Write file data */
    int32_t current_pos = PAK_HEADER_SIZE;
    for (int i = 0; i < pak->numfiles; i++) {
        if (pak->files[i].data) {
            if (fwrite(pak->files[i].data, pak->files[i].filelen, 1, f) != 1) {
                fprintf(stderr, "Error: Cannot write file data for '%s'\n", 
                        pak->files[i].filename);
                fclose(f);
                return false;
            }
        }
        pak->files[i].filepos = current_pos;
        current_pos += pak->files[i].filelen;
    }
    
    /* Write directory */
    for (int i = 0; i < pak->numfiles; i++) {
        pak_directory_entry_t entry;
        memset(entry.name, 0, PAK_MAX_FILENAME_LENGTH);
        strncpy(entry.name, pak->files[i].filename, PAK_MAX_FILENAME_LENGTH - 1);
        entry.filepos = htole32(pak->files[i].filepos);  /* Store in little-endian format (standard Quake PAK) */
        entry.filelen = htole32(pak->files[i].filelen);  /* Store in little-endian format (standard Quake PAK) */
        
        if (fwrite(&entry, sizeof(pak_directory_entry_t), 1, f) != 1) {
            fprintf(stderr, "Error: Cannot write directory entry for '%s'\n", 
                    pak->files[i].filename);
            fclose(f);
            return false;
        }
    }
    
    fclose(f);
    return true;
}

/* Free PAK structure */
void pak_free(pak_t *pak) {
    if (!pak) return;
    
    if (pak->filename) free(pak->filename);
    if (pak->handle) {
        fclose(pak->handle);
        pak->handle = NULL;
    }
    
    if (pak->files) {
        for (int i = 0; i < pak->numfiles; i++) {
            if (pak->files[i].filename) free(pak->files[i].filename);
            if (pak->files[i].data) free(pak->files[i].data);
        }
        free(pak->files);
    }
    
    free(pak);
}

/* Get number of files in PAK */
int pak_get_file_count(pak_t *pak) {
    return pak ? pak->numfiles : 0;
}

/* Check if file exists in PAK */
bool pak_file_exists(pak_t *pak, const char *filename) {
    if (!pak || !filename) return false;
    
    for (int i = 0; i < pak->numfiles; i++) {
        if (strcmp(pak->files[i].filename, filename) == 0) {
            return true;
        }
    }
    return false;
}

/* Get file info from PAK */
pak_file_t *pak_get_file(pak_t *pak, const char *filename) {
    if (!pak || !filename) return NULL;
    
    for (int i = 0; i < pak->numfiles; i++) {
        if (strcmp(pak->files[i].filename, filename) == 0) {
            return &pak->files[i];
        }
    }
    return NULL;
}

/* Load file data from PAK */
bool pak_load_file_data(pak_t *pak, const char *filename) {
    pak_file_t *file = pak_get_file(pak, filename);
    if (!file) return false;
    
    if (file->data) return true; /* Already loaded */
    
    if (!pak->handle) {
        pak->handle = fopen(pak->filename, "rb");
        if (!pak->handle) return false;
    }
    
    file->data = malloc(file->filelen);
    if (!file->data) return false;
    
    if (fseek(pak->handle, file->filepos, SEEK_SET) != 0) {
        free(file->data);
        file->data = NULL;
        return false;
    }
    
    if (fread(file->data, file->filelen, 1, pak->handle) != 1) {
        free(file->data);
        file->data = NULL;
        return false;
    }
    
    return true;
}

/* Extract single file from PAK */
bool pak_extract_file(pak_t *pak, const char *filename, const char *output_path) {
    pak_file_t *file = pak_get_file(pak, filename);
    if (!file) return false;
    
    if (!pak_load_file_data(pak, filename)) return false;
    
    FILE *f = fopen(output_path, "wb");
    if (!f) return false;
    
    bool success = (fwrite(file->data, file->filelen, 1, f) == 1);
    fclose(f);
    
    return success;
}

/* Extract all files from PAK */
bool pak_extract_all(pak_t *pak, const char *output_dir) {
    if (!pak || !output_dir) return false;
    
    /* Create output directory */
    if (!create_directory_recursive(output_dir)) {
        fprintf(stderr, "Error: Cannot create output directory '%s'\n", output_dir);
        return false;
    }
    
    for (int i = 0; i < pak->numfiles; i++) {
        char output_path[1024];
        /* Handle trailing slash in output_dir */
        if (output_dir[strlen(output_dir) - 1] == '/') {
            snprintf(output_path, sizeof(output_path), "%s%s", output_dir, pak->files[i].filename);
        } else {
            snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, pak->files[i].filename);
        }
        
        /* Create subdirectories if needed */
        char *last_slash = strrchr(output_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            if (!create_directory_recursive(output_path)) {
                fprintf(stderr, "Warning: Cannot create directory for '%s'\n", 
                        pak->files[i].filename);
            }
            *last_slash = '/';
        }
        
        if (!pak_extract_file(pak, pak->files[i].filename, output_path)) {
            fprintf(stderr, "Error: Cannot extract file '%s'\n", pak->files[i].filename);
            return false;
        }
    }
    
    return true;
}

/* Create new empty PAK */
pak_t *pak_create_new(void) {
    pak_t *pak = malloc(sizeof(pak_t));
    if (!pak) return NULL;
    
    pak->filename = NULL;
    pak->numfiles = 0;
    pak->files = NULL;
    pak->handle = NULL;
    
    return pak;
}

/* Add file to PAK from disk */
bool pak_add_file(pak_t *pak, const char *filename, const char *pak_path) {
    FILE *f = fopen(filename, "rb");
    if (!f) return false;
    
    /* Get file size */
    fseek(f, 0, SEEK_END);
    int32_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    /* Read file data */
    uint8_t *data = malloc(size);
    if (!data) {
        fclose(f);
        return false;
    }
    
    if (fread(data, size, 1, f) != 1) {
        free(data);
        fclose(f);
        return false;
    }
    
    fclose(f);
    
    /* Add to PAK */
    bool success = pak_add_file_data(pak, pak_path, data, size);
    free(data);
    return success;
}

/* Add file data to PAK */
bool pak_add_file_data(pak_t *pak, const char *pak_path, const uint8_t *data, int32_t size) {
    if (!pak || !pak_path || !data || size < 0) return false;
    
    /* Check if file already exists */
    if (pak_file_exists(pak, pak_path)) {
        fprintf(stderr, "Warning: File '%s' already exists in PAK\n", pak_path);
        return false;
    }
    
    /* Reallocate file array */
    pak_file_t *new_files = realloc(pak->files, (pak->numfiles + 1) * sizeof(pak_file_t));
    if (!new_files) return false;
    
    pak->files = new_files;
    
    /* Add new file */
    int idx = pak->numfiles++;
    pak->files[idx].filename = strdup(pak_path);
    pak->files[idx].filelen = size;
    pak->files[idx].filepos = 0; /* Will be set when saving */
    pak->files[idx].data = malloc(size);
    
    if (!pak->files[idx].filename || !pak->files[idx].data) {
        pak->numfiles--; /* Rollback */
        return false;
    }
    
    memcpy(pak->files[idx].data, data, size);
    return true;
}

/* Remove file from PAK */
bool pak_remove_file(pak_t *pak, const char *filename) {
    if (!pak || !filename) return false;
    
    for (int i = 0; i < pak->numfiles; i++) {
        if (strcmp(pak->files[i].filename, filename) == 0) {
            /* Free file resources */
            free(pak->files[i].filename);
            if (pak->files[i].data) free(pak->files[i].data);
            
            /* Shift remaining files */
            for (int j = i; j < pak->numfiles - 1; j++) {
                pak->files[j] = pak->files[j + 1];
            }
            
            pak->numfiles--;
            return true;
        }
    }
    
    return false;
}

/* Print PAK information */
void pak_print_info(pak_t *pak) {
    if (!pak) return;
    
    printf("PAK File: %s\n", pak->filename ? pak->filename : "(new)");
    printf("Files: %d\n", pak->numfiles);
    printf("Directory offset: %d\n", pak->header.dirofs);
    printf("Directory length: %d\n", pak->header.dirlen);
    printf("Total size: %d bytes\n", pak_calculate_size(pak));
}

/* List all files in PAK */
void pak_list_files(pak_t *pak) {
    if (!pak) return;
    
    printf("Files in PAK:\n");
    for (int i = 0; i < pak->numfiles; i++) {
        printf("  %s (%d bytes)\n", pak->files[i].filename, pak->files[i].filelen);
    }
}

/* Calculate total PAK size */
int32_t pak_calculate_size(pak_t *pak) {
    if (!pak) return 0;
    
    int32_t size = PAK_HEADER_SIZE + (pak->numfiles * PAK_DIRECTORY_ENTRY_SIZE);
    for (int i = 0; i < pak->numfiles; i++) {
        size += pak->files[i].filelen;
    }
    return size;
}

/* Helper function to recursively scan directory and add files */
static bool scan_directory_recursive(pak_t *pak, const char *dir_path, const char *base_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open directory %s\n", dir_path);
        return false;
    }
    
    struct dirent *entry;
    bool success = true;
    
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Build full path */
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        /* Build PAK path (relative to base directory) */
        char pak_path[1024];
        if (base_path && strlen(base_path) > 0) {
            snprintf(pak_path, sizeof(pak_path), "%s/%s", base_path, entry->d_name);
        } else {
            strncpy(pak_path, entry->d_name, sizeof(pak_path) - 1);
            pak_path[sizeof(pak_path) - 1] = '\0';
        }
        
        /* Check if it's a directory */
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                /* Recursively scan subdirectory */
                if (!scan_directory_recursive(pak, full_path, pak_path)) {
                    success = false;
                }
            } else if (S_ISREG(st.st_mode)) {
                /* Add regular file */
                printf("Adding file: %s -> %s\n", full_path, pak_path);
                if (!pak_add_file(pak, full_path, pak_path)) {
                    fprintf(stderr, "Error: Failed to add file %s\n", full_path);
                    success = false;
                }
            }
        }
    }
    
    closedir(dir);
    return success;
}

/* Add entire directory contents to PAK */
bool pak_add_directory(pak_t *pak, const char *directory_path) {
    if (!pak || !directory_path) {
        return false;
    }
    
    /* Check if directory exists */
    struct stat st;
    if (stat(directory_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: Directory %s does not exist or is not a directory\n", directory_path);
        return false;
    }
    
    printf("Adding directory contents: %s\n", directory_path);
    
    /* Scan directory recursively */
    return scan_directory_recursive(pak, directory_path, "");
}
