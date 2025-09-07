# Music Metadata System

The Carnifex Engine features a sophisticated music metadata system that automatically extracts and displays information about playing music files.

## [OVERVIEW]

The music metadata system provides:
- **Automatic metadata extraction** from audio files
- **Console logging** with formatted output
- **Colored toaster notifications** on screen
- **Menu configuration** options
- **Fallback support** for files without metadata

## [FEATURES]

### Metadata Extraction
- **OGG Vorbis Comments**: Full support for TITLE, ARTIST, ALBUM, DATE
- **MP3 ID3v1 Tags**: Complete ID3v1 tag support
- **Fallback System**: Shows filename when no metadata is available

### Display Options
- **Console Output**: `MUSIC: [Title] by [Artist]`
- **Toaster Notifications**: On-screen display with colors
  - Title: Dark white (0.8, 0.8, 0.8)
  - Artist: Light red (1.0, 0.4, 0.4)
  - Duration: 5 seconds (configurable)

### Configuration
- **CVars**: Runtime configuration via console variables
- **Menu Options**: User-friendly toggle switches
- **Persistent Settings**: Configuration saved to config files

## [IMPLEMENTATION]

### Core Files

#### `core/music_metadata.c`
Main implementation file containing:
- Metadata extraction functions
- Display formatting
- Toaster notification system
- CVar registration

#### `core/music_metadata.h`
Header file with:
- Function declarations
- Data structures
- Constants and definitions

### Integration Points

#### `core/bgmusic.c`
- Calls metadata extraction on music load
- Integrates with both automatic and manual music playback
- Supports console commands and track-based playback

#### `core/gl_screen.c`
- Renders toaster notifications
- Handles screen positioning and timing

#### `core/menu.c`
- Provides menu options for configuration
- Toggle switches for console and toaster display

## [USAGE]

### Console Commands

```bash
# Play music with metadata display
music track02
music track03

# Configure display options
music_info_log 1          # Enable console logging
music_info_toaster 1      # Enable toaster notifications
music_info_toaster_duration 5  # Set duration to 5 seconds
```

### Menu Configuration

1. **Options → Game → Music Info Log**: Toggle console output
2. **Options → Game → Music Info Toaster**: Toggle on-screen display

### Automatic Playback

The system automatically displays metadata when:
- Maps load background music
- Demos play music tracks
- Any automatic music playback occurs

## [VISUAL DESIGN]

### Toaster Notifications

The toaster system displays:
- **Background**: Semi-transparent black box
- **Position**: Centered at top of screen (y=100)
- **Text Layout**: Title + " by " + Artist
- **Colors**: 
  - Title: Dark white for readability
  - " by ": Dark white to match title
  - Artist: Light red for distinction
- **Duration**: 5 seconds (configurable)

### Console Output

Console messages follow the format:
```
MUSIC: Gallows Pole Awaits by Edward 'Toy' Facundo
```

## [CONFIGURATION VARIABLES]

### `music_info_log`
- **Type**: Integer (0/1)
- **Default**: 1
- **Description**: Enable/disable console logging
- **Usage**: `music_info_log 0` to disable

### `music_info_toaster`
- **Type**: Integer (0/1)
- **Default**: 1
- **Description**: Enable/disable toaster notifications
- **Usage**: `music_info_toaster 0` to disable

### `music_info_toaster_duration`
- **Type**: Float
- **Default**: 5.0
- **Description**: Toaster display duration in seconds
- **Usage**: `music_info_toaster_duration 3` for 3 seconds

## [FILE SUPPORT]

### Supported Formats

| Format | Extension | Metadata Support | Notes |
|--------|-----------|------------------|-------|
| OGG Vorbis | .ogg | Full | TITLE, ARTIST, ALBUM, DATE |
| MP3 | .mp3 | ID3v1 | Basic tag support |
| WAV | .wav | None | Filename fallback |
| Module | .it, .s3m, .xm, .mod, .umx | None | Filename fallback |

### Metadata Tags

#### OGG Vorbis Comments
- `TITLE`: Song title
- `ARTIST`: Artist name
- `ALBUM`: Album name
- `DATE`: Release year

#### MP3 ID3v1 Tags
- Title (30 characters)
- Artist (30 characters)
- Album (30 characters)
- Year (4 characters)

## [ADDING MUSIC WITH METADATA]

### OGG Files

Use `vorbiscomment` to add metadata:

```bash
# Add complete metadata
vorbiscomment -t "TITLE=Your Song Title" \
              -t "ARTIST=Your Artist Name" \
              -t "ALBUM=Your Album" \
              -t "DATE=2025" \
              your_file.ogg

# View existing metadata
vorbiscomment -l your_file.ogg
```

### MP3 Files

Use any MP3 tag editor (e.g., `id3v2`, `mp3tag`) to add ID3v1 tags:

```bash
# Using id3v2
id3v2 -a "Artist Name" -A "Album Name" -t "Song Title" -y 2025 your_file.mp3
```

### File Placement

Place music files in the game's `music/` directory:
```
carnifex-game/
└── music/
    ├── track02.ogg
    ├── track03.ogg
    └── ...
```

## [TECHNICAL DETAILS]

### Metadata Extraction Process

1. **File Detection**: System detects audio file format
2. **Format-Specific Parsing**: 
   - OGG: Direct string search for Vorbis comments
   - MP3: ID3v1 tag parsing
3. **Data Validation**: Clean and validate extracted strings
4. **Fallback**: Use filename if no metadata found

### Display System

1. **Console Logging**: Format and print to console
2. **Toaster Preparation**: Store title/artist separately for colored display
3. **Rendering**: Draw colored text with proper positioning
4. **Timing**: Manage display duration and cleanup

### Performance Considerations

- **Lazy Loading**: Metadata extracted only when needed
- **Caching**: No caching implemented (extracts on each play)
- **Memory Usage**: Minimal memory footprint
- **File I/O**: Efficient file reading with limited buffer sizes

## [TROUBLESHOOTING]

### Common Issues

1. **No Metadata Displayed**:
   - Check if file has proper metadata tags
   - Verify file format is supported
   - Ensure CVars are enabled

2. **Toaster Not Visible**:
   - Check `music_info_toaster` CVar
   - Verify toaster duration setting
   - Ensure screen positioning is correct

3. **Console Output Missing**:
   - Check `music_info_log` CVar
   - Verify console is open and visible
   - Check for console filtering

### Debug Information

Enable debug mode for additional information:
```bash
./build-artifacts/carnifex-engine -game carnifex-game -dev
```

## [FUTURE ENHANCEMENTS]

Potential improvements for the music metadata system:

1. **Additional Formats**: FLAC, Opus metadata support
2. **Enhanced Tags**: Genre, track number, album art
3. **Caching System**: Cache metadata to avoid repeated file I/O
4. **Playlist Support**: Track listing and navigation
5. **Visual Enhancements**: Album art display, progress bars
6. **Audio Analysis**: BPM detection, waveform display

## [RELATED DOCUMENTATION]

- [Audio Formats](AUDIO_FORMATS.md) - Supported audio formats
- [Audio Quick Reference](AUDIO_QUICK_REFERENCE.md) - Quick audio guide
- [Engine Guide](../user/CARNIFEX_ENGINE_GUIDE.md) - Complete engine documentation
- [Game Template](../user/CARNIFEX_GAME_TEMPLATE.md) - Game development guide
