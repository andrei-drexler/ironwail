# Carnifex Development Tools

This document describes the development tools created for the Carnifex Quake engine, including LMP file generation and audio preparation tools.

## Overview

We've created a comprehensive library and CLI tool system for generating and manipulating LMP files, particularly conchars (console fonts) for the Carnifex engine. We've also added audio preparation tools for proper game asset deployment.

## Documentation

- **[AUDIO_FORMATS.md](AUDIO_FORMATS.md)** - Complete guide to audio formats and PAK deployment
- **[AUDIO_QUICK_REFERENCE.md](AUDIO_QUICK_REFERENCE.md)** - Quick reference for audio preparation
- **[TOOLS_README.md](TOOLS_README.md)** - This file (LMP and general tools)

## Directory Structure

```
carnifex/
├── lib/                    # Core LMP library
│   ├── lmp.h              # Header file with API definitions
│   └── lmp.c              # Implementation of LMP file handling
├── tools/                  # CLI tools
│   ├── carnifex-cli.c     # Main CLI tool (includes audio processing)
│   ├── Makefile           # Build system for tools
│   ├── scripts/           # Example scripts
│   │   └── audio_examples.sh   # Usage examples
│   └── README.md          # Tool-specific documentation
└── CMakeLists.txt         # Updated with tool integration
```

## Features

### LMP Library (`lib/`)

- **General LMP Support**: Read/write raw LMP files (Quake format)
- **Conchars Support**: Specialised handling for console fonts
- **Character Access**: Extract and modify individual 8×8 characters
- **Hash Detection**: Automatically detect custom vs. original conchars
- **Memory Management**: Proper allocation and cleanup

### CLI Tool (`tools/carnifex-cli`)

- **File Information**: Display detailed information about LMP files
- **Character Extraction**: Extract all 256 characters to individual files
- **File Conversion**: Convert between different LMP formats
- **Auto-detection**: Automatically detect file types when possible
- **PAK Support**: Create and extract PAK files for game deployment

### Audio Preparation (Integrated in CLI Tool)

- **Audio Conversion**: Convert audio files to engine-compatible formats
- **Format Validation**: Validate audio files for engine compatibility
- **Directory Structure**: Create standard Carnifex audio directory structure
- **Unified Interface**: All audio operations integrated into the main CLI tool

## Usage Examples

### Show File Information
```bash
./tools/carnifex-cli -i carnifex/gfx/conchars.lmp -I
```

### Extract All Characters
```bash
./tools/carnifex-cli -i carnifex/gfx/conchars.lmp -e -o extracted_chars/
```

### Copy/Convert Files
```bash
./tools/carnifex-cli -i input.lmp -o output.lmp -t conchars
```

### Audio Preparation
```bash
# Convert single audio file to sound effect
./tools/carnifex-cli -A -i input.wav -o sound/menu1.wav -T sound_effect

# Convert single audio file to music
./tools/carnifex-cli -A -i music.wav -o music/track01.wav -T music -F mp3

# Validate audio files
./tools/carnifex-cli -V -i sound/

# Create directory structure
./tools/carnifex-cli -S -o carnifex/

# Create PAK file
./tools/carnifex-cli -c -i carnifex/ -o carnifex.pak
```

## Building

### Using CMake (Recommended)
```bash
make carnifex-cli
```

### Using Tools Makefile
```bash
cd tools
make
```

## Integration with Carnifex Engine

The tools are fully compatible with the Carnifex engine's LMP handling:

- **File Format**: Generates standard Quake LMP files (128×128, 8-bit indexed)
- **Transparency**: Proper handling of transparent pixels (indices 0 and 255)
- **Custom Detection**: Engine automatically detects custom conchars
- **Loading**: Works with both WAD and individual file loading

## Technical Details

### Conchars Format
- **Size**: 128×128 pixels (16,384 bytes)
- **Layout**: 16×16 grid of 8×8 pixel characters
- **Colour**: 8-bit indexed colour using Quake palette
- **Transparency**: Index 0 and 255 are transparent

### Hash Detection
The engine uses CRC32 hash comparison to detect original Quake conchars:
- **Original Hash**: `0xc7e2a10a`
- **Custom Detection**: Any hash different from original

### Character Layout
```
Character 0:  (0,0)   Character 1:  (8,0)   ... Character 15:  (120,0)
Character 16: (0,8)   Character 17: (8,8)   ... Character 31:  (120,8)
...
Character 240:(0,120) Character 241:(8,120) ... Character 255: (120,120)
```

## Future Enhancements

Potential improvements for the tools:

1. **Image Import**: Convert PNG/BMP images to conchars format
2. **Palette Support**: Handle custom palettes for conchars
3. **Batch Processing**: Process multiple files at once
4. **GUI Interface**: Graphical tool for visual editing
5. **Other LMP Types**: Support for other LMP file types (textures, sprites)

## API Reference

### Core Functions

```c
// General LMP functions
lmp_file_t *lmp_create(uint32_t width, uint32_t height);
lmp_file_t *lmp_load_from_file(const char *filename);
bool lmp_save_to_file(const lmp_file_t *lmp, const char *filename);
void lmp_free(lmp_file_t *lmp);

// Conchars specific functions
conchars_t *conchars_create(void);
conchars_t *conchars_load_from_file(const char *filename);
bool conchars_save_to_file(const conchars_t *conchars, const char *filename);
void conchars_free(conchars_t *conchars);

// Character access
bool conchars_get_character(const conchars_t *conchars, int char_index, uint8_t *dest);
bool conchars_set_character(conchars_t *conchars, int char_index, const uint8_t *src);
```

This tool system provides a solid foundation for working with LMP files in the Carnifex project and can be easily extended for additional file types and functionality.
