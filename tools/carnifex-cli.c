/*
 * Carnifex CLI Tool
 * 
 * Command-line interface for generating and manipulating LMP files
 * for the Carnifex Quake engine.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "lib/lmp.h"
#include "lib/pak.h"

/* Command line options */
static struct option long_options[] = {
    {"help",        no_argument,       0, 'h'},
    {"version",     no_argument,       0, 'v'},
    {"input",       required_argument, 0, 'i'},
    {"output",      required_argument, 0, 'o'},
    {"type",        required_argument, 0, 't'},
    {"info",        no_argument,       0, 'I'},
    {"extract",     no_argument,       0, 'e'},
    {"create",      no_argument,       0, 'c'},
    {"list",        no_argument,       0, 'l'},
    {"add",         required_argument, 0, 'a'},
    {"directory",   required_argument, 0, 'd'},
    {"convert",     required_argument, 0, 'C'},
    {0, 0, 0, 0}
};

/* Global variables */
static char *input_file = NULL;
static char *output_file = NULL;
static char *file_type = NULL;
static bool show_info = false;
static bool extract_mode = false;
static bool create_mode = false;
static bool list_mode = false;
static char **add_files = NULL;
static int add_files_count = 0;
static char *directory_path = NULL;
static char *convert_format = NULL;

/* Function prototypes */
void print_usage(const char *program_name);
void print_version(void);
void print_info(const char *filename);
bool process_conchars(void);
bool extract_conchars(void);
bool process_pak(void);
bool extract_pak(void);
bool create_pak(void);
bool list_pak(void);
bool process_lmp(void);
bool extract_lmp(void);
bool convert_lmp(void);

int main(int argc, char *argv[]) {
    int option_index = 0;
    int c;
    
    /* Parse command line arguments */
    while ((c = getopt_long(argc, argv, "hvi:o:t:Iecla:d:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                print_usage(argv[0]);
                return 0;
                
            case 'v':
                print_version();
                return 0;
                
            case 'i':
                input_file = optarg;
                break;
                
            case 'o':
                output_file = optarg;
                break;
                
            case 't':
                file_type = optarg;
                break;
                
            case 'I':
                show_info = true;
                break;
                
            case 'e':
                extract_mode = true;
                break;
                
            case 'c':
                create_mode = true;
                break;
                
            case 'l':
                list_mode = true;
                break;
                
            case 'a':
                add_files = realloc(add_files, (add_files_count + 1) * sizeof(char*));
                if (add_files) {
                    add_files[add_files_count] = strdup(optarg);
                    add_files_count++;
                }
                break;
                
            case 'd':
                directory_path = optarg;
                break;
                
            case 'C':
                convert_format = optarg;
                break;
                
            case '?':
                print_usage(argv[0]);
                return 1;
                
            default:
                abort();
        }
    }
    
    /* Validate arguments */
    if (!input_file && !show_info && !create_mode) {
        fprintf(stderr, "Error: Input file required (use -i or --input)\n");
        print_usage(argv[0]);
        return 1;
    }
    
    if (show_info) {
        if (!input_file) {
            fprintf(stderr, "Error: Input file required for info mode\n");
            return 1;
        }
        print_info(input_file);
        return 0;
    }
    
    /* For extraction mode, try to auto-detect file type if not specified */
    if (extract_mode && !file_type) {
        /* Try to load as PAK first */
        if (pak_is_valid(input_file)) {
            file_type = "pak";
        } else {
            /* Try to load as general LMP first */
            lmp_file_t *lmp = lmp_load_from_file(input_file);
            if (lmp) {
                /* Check if it's a conchars file */
                if (lmp->width == CONCHARS_WIDTH && lmp->height == CONCHARS_HEIGHT) {
                    file_type = "conchars";
                } else {
                    file_type = "lmp";
                }
                lmp_free(lmp);
            }
        }
    }
    
    /* For list mode, try to auto-detect file type if not specified */
    if (list_mode && !file_type) {
        if (pak_is_valid(input_file)) {
            file_type = "pak";
        }
    }
    
    /* Handle create mode */
    if (create_mode) {
        if (!file_type) {
            file_type = "pak"; /* Default to PAK for creation */
        }
        if (strcmp(file_type, "pak") == 0) {
            if (!create_pak()) {
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Can only create PAK files\n");
            return 1;
        }
        return 0;
    }
    
    /* Handle convert mode */
    if (convert_format) {
        if (!input_file || !output_file) {
            fprintf(stderr, "Error: Input and output files required for conversion\n");
            print_usage(argv[0]);
            return 1;
        }
        if (!convert_lmp()) {
            return 1;
        }
        return 0;
    }
    
    /* Process based on file type */
    if (!file_type) {
        fprintf(stderr, "Error: File type required (use -t or --type)\n");
        print_usage(argv[0]);
        return 1;
    }
    
    if (strcmp(file_type, "conchars") == 0) {
        if (extract_mode) {
            if (!extract_conchars()) {
                return 1;
            }
        } else {
            if (!process_conchars()) {
                return 1;
            }
        }
    } else if (strcmp(file_type, "pak") == 0) {
        if (list_mode) {
            if (!list_pak()) {
                return 1;
            }
        } else if (extract_mode) {
            if (!extract_pak()) {
                return 1;
            }
        } else {
            if (!process_pak()) {
                return 1;
            }
        }
    } else if (strcmp(file_type, "lmp") == 0) {
        if (extract_mode) {
            if (!extract_lmp()) {
                return 1;
            }
        } else {
            if (!process_lmp()) {
                return 1;
            }
        }
    } else {
        fprintf(stderr, "Error: Unsupported file type '%s'\n", file_type);
        fprintf(stderr, "Supported types: conchars, pak, lmp\n");
        return 1;
    }
    
    /* Cleanup */
    if (add_files) {
        for (int i = 0; i < add_files_count; i++) {
            free(add_files[i]);
        }
        free(add_files);
    }
    
    return 0;
}

void print_usage(const char *program_name) {
    printf("Carnifex CLI Tool - LMP and PAK file generator for Carnifex Quake engine\n\n");
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --version           Show version information\n");
    printf("  -i, --input FILE        Input file path\n");
    printf("  -o, --output FILE       Output file path\n");
    printf("  -t, --type TYPE         File type (conchars, pak, lmp)\n");
    printf("  -I, --info              Show file information\n");
    printf("  -e, --extract           Extract files from archive\n");
    printf("  -c, --create            Create new archive\n");
    printf("  -l, --list              List files in archive\n");
    printf("  -a, --add FILE          Add file to archive (use with -c)\n");
    printf("  -d, --directory DIR     Add entire directory contents to archive (use with -c)\n");
    printf("  -C, --convert FORMAT    Convert between formats (pcx: LMP->PCX, lmp: PCX->LMP)\n\n");
    printf("Examples:\n");
    printf("  %s -i conchars.lmp -I                    # Show conchars info\n", program_name);
    printf("  %s -i image.png -o conchars.lmp -t conchars  # Convert image to conchars\n", program_name);
    printf("  %s -i conchars.lmp -e -o chars/         # Extract all characters\n", program_name);
    printf("  %s -i pak0.pak -l                       # List files in PAK\n", program_name);
    printf("  %s -i pak0.pak -e -o extracted/         # Extract all files from PAK\n", program_name);
    printf("  %s -i sp_menu.lmp -e -o extracted/      # Extract LMP file to raw data\n", program_name);
    printf("  %s -i sp_menu.lmp -o sp_menu.pcx --convert pcx # Convert LMP to PCX\n", program_name);
    printf("  %s -i sp_menu.pcx -o sp_menu.lmp --convert lmp # Convert PCX to LMP\n", program_name);
    printf("  %s -c -o new.pak -t pak -a file1.txt -a file2.txt  # Create new PAK\n", program_name);
    printf("  %s -c -o music.pak -t pak -d music/     # Create PAK from directory\n", program_name);
}

void print_version(void) {
    printf("Carnifex CLI Tool v1.0.0\n");
    printf("LMP file generator for Carnifex Quake engine\n");
}

void print_info(const char *filename) {
    printf("File: %s\n", filename);
    
    /* Try to load as PAK first */
    if (pak_is_valid(filename)) {
        pak_t *pak = pak_load_from_file(filename);
        if (pak) {
            printf("Type: PAK (Quake archive)\n");
            printf("Files: %d\n", pak->numfiles);
            printf("Directory offset: %d\n", pak->header.dirofs);
            printf("Directory length: %d\n", pak->header.dirlen);
            printf("Total size: %d bytes\n", pak_calculate_size(pak));
            
            pak_free(pak);
            return;
        }
    }
    
    /* Try to load as conchars */
    conchars_t *conchars = conchars_load_from_file(filename);
    if (conchars) {
        printf("Type: Conchars (console font)\n");
        printf("Size: %dx%d pixels\n", CONCHARS_WIDTH, CONCHARS_HEIGHT);
        printf("Characters: %d (16x16 grid)\n", CONCHARS_TOTAL_CHARS);
        printf("Character size: %dx%d pixels\n", CONCHARS_CHAR_WIDTH, CONCHARS_CHAR_HEIGHT);
        printf("Custom: %s\n", conchars->is_custom ? "Yes" : "No (original Quake)");
        
        conchars_free(conchars);
        return;
    }
    
    /* Try to load as general LMP */
    lmp_file_t *lmp = lmp_load_from_file(filename);
    if (lmp) {
        printf("Type: LMP (generic)\n");
        printf("Size: %dx%d pixels\n", lmp->width, lmp->height);
        printf("Data size: %zu bytes\n", lmp->data_size);
        
        lmp_free(lmp);
        return;
    }
    
    printf("Type: Unknown or invalid file\n");
}

bool process_conchars(void) {
    if (!output_file) {
        fprintf(stderr, "Error: Output file required for processing\n");
        return false;
    }
    
    printf("Processing conchars from %s to %s\n", input_file, output_file);
    
    /* For now, just copy the input to output */
    /* TODO: Add image conversion functionality */
    conchars_t *conchars = conchars_load_from_file(input_file);
    if (!conchars) {
        fprintf(stderr, "Error: Failed to load conchars from %s\n", input_file);
        return false;
    }
    
    if (!conchars_save_to_file(conchars, output_file)) {
        fprintf(stderr, "Error: Failed to save conchars to %s\n", output_file);
        conchars_free(conchars);
        return false;
    }
    
    printf("Successfully processed conchars\n");
    conchars_free(conchars);
    return true;
}

bool extract_conchars(void) {
    if (!output_file) {
        fprintf(stderr, "Error: Output directory required for extraction\n");
        return false;
    }
    
    printf("Extracting conchars from %s to %s\n", input_file, output_file);
    
    conchars_t *conchars = conchars_load_from_file(input_file);
    if (!conchars) {
        fprintf(stderr, "Error: Failed to load conchars from %s\n", input_file);
        return false;
    }
    
    /* Create output directory if it doesn't exist */
    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", output_file);
    if (system(mkdir_cmd) != 0) {
        fprintf(stderr, "Warning: Failed to create output directory\n");
    }
    
    /* Extract each character */
    uint8_t char_data[CONCHARS_CHAR_WIDTH * CONCHARS_CHAR_HEIGHT];
    char filename[256];
    
    for (int i = 0; i < CONCHARS_TOTAL_CHARS; i++) {
        if (!conchars_get_character(conchars, i, char_data)) {
            fprintf(stderr, "Error: Failed to extract character %d\n", i);
            conchars_free(conchars);
            return false;
        }
        
        snprintf(filename, sizeof(filename), "%s/char_%03d.raw", output_file, i);
        
        FILE *f = fopen(filename, "wb");
        if (!f) {
            fprintf(stderr, "Error: Failed to create file %s\n", filename);
            conchars_free(conchars);
            return false;
        }
        
        if (fwrite(char_data, 1, CONCHARS_CHAR_WIDTH * CONCHARS_CHAR_HEIGHT, f) != 
            CONCHARS_CHAR_WIDTH * CONCHARS_CHAR_HEIGHT) {
            fprintf(stderr, "Error: Failed to write character %d\n", i);
            fclose(f);
            conchars_free(conchars);
            return false;
        }
        
        fclose(f);
    }
    
    printf("Successfully extracted %d characters\n", CONCHARS_TOTAL_CHARS);
    conchars_free(conchars);
    return true;
}

bool process_pak(void) {
    if (!output_file) {
        fprintf(stderr, "Error: Output file required for processing\n");
        return false;
    }
    
    printf("Processing PAK from %s to %s\n", input_file, output_file);
    
    /* For now, just copy the input to output */
    pak_t *pak = pak_load_from_file(input_file);
    if (!pak) {
        fprintf(stderr, "Error: Failed to load PAK from %s\n", input_file);
        return false;
    }
    
    if (!pak_save_to_file(pak, output_file)) {
        fprintf(stderr, "Error: Failed to save PAK to %s\n", output_file);
        pak_free(pak);
        return false;
    }
    
    printf("Successfully processed PAK\n");
    pak_free(pak);
    return true;
}

bool extract_pak(void) {
    if (!output_file) {
        fprintf(stderr, "Error: Output directory required for extraction\n");
        return false;
    }
    
    printf("Extracting PAK from %s to %s\n", input_file, output_file);
    
    pak_t *pak = pak_load_from_file(input_file);
    if (!pak) {
        fprintf(stderr, "Error: Failed to load PAK from %s\n", input_file);
        return false;
    }
    
    if (!pak_extract_all(pak, output_file)) {
        fprintf(stderr, "Error: Failed to extract PAK files\n");
        pak_free(pak);
        return false;
    }
    
    printf("Successfully extracted %d files\n", pak->numfiles);
    pak_free(pak);
    return true;
}

bool create_pak(void) {
    if (!output_file) {
        fprintf(stderr, "Error: Output file required for PAK creation\n");
        return false;
    }
    
    printf("Creating PAK file: %s\n", output_file);
    
    pak_t *pak = pak_create_new();
    if (!pak) {
        fprintf(stderr, "Error: Failed to create PAK structure\n");
        return false;
    }
    
    /* Add directory contents if specified */
    if (directory_path) {
        if (!pak_add_directory(pak, directory_path)) {
            fprintf(stderr, "Error: Failed to add directory %s\n", directory_path);
            pak_free(pak);
            return false;
        }
    }
    
    /* Add individual files if specified */
    for (int i = 0; i < add_files_count; i++) {
        printf("Adding file: %s\n", add_files[i]);
        if (!pak_add_file(pak, add_files[i], add_files[i])) {
            fprintf(stderr, "Error: Failed to add file %s\n", add_files[i]);
            pak_free(pak);
            return false;
        }
    }
    
    /* Check if any files were added */
    if (pak->numfiles == 0) {
        fprintf(stderr, "Error: No files to add to PAK. Use -a or -d options.\n");
        pak_free(pak);
        return false;
    }
    
    /* Save PAK file */
    if (!pak_save_to_file(pak, output_file)) {
        fprintf(stderr, "Error: Failed to save PAK file\n");
        pak_free(pak);
        return false;
    }
    
    printf("Successfully created PAK with %d files\n", pak->numfiles);
    pak_free(pak);
    return true;
}

bool list_pak(void) {
    printf("Listing PAK file: %s\n", input_file);
    
    pak_t *pak = pak_load_from_file(input_file);
    if (!pak) {
        fprintf(stderr, "Error: Failed to load PAK from %s\n", input_file);
        return false;
    }
    
    pak_print_info(pak);
    pak_list_files(pak);
    
    pak_free(pak);
    return true;
}

bool process_lmp(void) {
    printf("Processing LMP file: %s\n", input_file);
    
    lmp_file_t *lmp = lmp_load_from_file(input_file);
    if (!lmp) {
        fprintf(stderr, "Error: Failed to load LMP from %s\n", input_file);
        return false;
    }
    
    printf("LMP Info:\n");
    printf("  Width: %d\n", lmp->width);
    printf("  Height: %d\n", lmp->height);
    printf("  Data size: %zu bytes\n", lmp->data_size);
    
    if (output_file) {
        printf("Saving LMP to: %s\n", output_file);
        if (!lmp_save_to_file(lmp, output_file)) {
            fprintf(stderr, "Error: Failed to save LMP to %s\n", output_file);
            lmp_free(lmp);
            return false;
        }
        printf("LMP saved successfully\n");
    }
    
    lmp_free(lmp);
    return true;
}

bool extract_lmp(void) {
    printf("Extracting LMP file: %s\n", input_file);
    
    lmp_file_t *lmp = lmp_load_from_file(input_file);
    if (!lmp) {
        fprintf(stderr, "Error: Failed to load LMP from %s\n", input_file);
        return false;
    }
    
    if (!output_file) {
        fprintf(stderr, "Error: Output directory required for extraction (use -o)\n");
        lmp_free(lmp);
        return false;
    }
    
    /* Create output directory if it doesn't exist */
    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", output_file);
    system(mkdir_cmd);
    
    /* Save LMP data as raw image data */
    char output_path[512];
    snprintf(output_path, sizeof(output_path), "%s/%s.raw", output_file, 
             strrchr(input_file, '/') ? strrchr(input_file, '/') + 1 : input_file);
    
    FILE *f = fopen(output_path, "wb");
    if (!f) {
        fprintf(stderr, "Error: Failed to create output file %s\n", output_path);
        lmp_free(lmp);
        return false;
    }
    
    /* Write width and height as header */
    fwrite(&lmp->width, sizeof(uint32_t), 1, f);
    fwrite(&lmp->height, sizeof(uint32_t), 1, f);
    
    /* Write pixel data */
    if (fwrite(lmp->data, lmp->data_size, 1, f) != 1) {
        fprintf(stderr, "Error: Failed to write LMP data\n");
        fclose(f);
        lmp_free(lmp);
        return false;
    }
    
    fclose(f);
    
    printf("LMP extracted successfully to: %s\n", output_path);
    printf("  Width: %d, Height: %d, Size: %zu bytes\n", 
           lmp->width, lmp->height, lmp->data_size);
    
    lmp_free(lmp);
    return true;
}

bool convert_lmp(void) {
    printf("Converting file: %s to %s format\n", input_file, convert_format);
    
    if (strcmp(convert_format, "pcx") == 0) {
        /* LMP to PCX conversion */
        lmp_file_t *lmp = lmp_load_from_file(input_file);
        if (!lmp) {
            fprintf(stderr, "Error: Failed to load LMP from %s\n", input_file);
            return false;
        }
        
        printf("LMP Info: %dx%d pixels, %zu bytes\n", lmp->width, lmp->height, lmp->data_size);
        printf("Converting to PCX format: %s\n", output_file);
        
        if (!lmp_to_pcx(lmp, output_file)) {
            fprintf(stderr, "Error: Failed to convert LMP to PCX\n");
            lmp_free(lmp);
            return false;
        }
        
        printf("PCX conversion successful\n");
        lmp_free(lmp);
        
    } else if (strcmp(convert_format, "lmp") == 0) {
        /* PCX to LMP conversion */
        lmp_file_t *lmp = pcx_to_lmp(input_file);
        if (!lmp) {
            fprintf(stderr, "Error: Failed to load PCX from %s\n", input_file);
            return false;
        }
        
        printf("PCX Info: %dx%d pixels, %zu bytes\n", lmp->width, lmp->height, lmp->data_size);
        printf("Converting to LMP format: %s\n", output_file);
        
        if (!lmp_save_to_file(lmp, output_file)) {
            fprintf(stderr, "Error: Failed to convert PCX to LMP\n");
            lmp_free(lmp);
            return false;
        }
        
        printf("LMP conversion successful\n");
        lmp_free(lmp);
        
    } else {
        fprintf(stderr, "Error: Unsupported conversion format: %s\n", convert_format);
        fprintf(stderr, "Supported formats: pcx (LMP->PCX), lmp (PCX->LMP)\n");
        return false;
    }
    
    return true;
}
