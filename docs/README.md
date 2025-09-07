# Carnifex Documentation

This directory contains all documentation for the Carnifex project.

## [DIRECTORY STRUCTURE]

```
docs/
├── development/           # Development documentation
│   ├── AUDIO_FORMATS.md
│   ├── AUDIO_QUICK_REFERENCE.md
│   ├── MINIMAL_GAME_SUMMARY.md
│   └── TOOLS_README.md
├── localization/          # Localization files
│   └── loc_english.txt
├── user/                  # User guides and documentation
│   ├── CARNIFEX_CREDITS.md
│   ├── CARNIFEX_ENGINE_GUIDE.md
│   ├── CARNIFEX_ENGINE_SUMMARY.md
│   └── CARNIFEX_GAME_TEMPLATE.md
├── PROJECT_STRUCTURE.md   # Complete project structure overview
├── Quakespasm-Music.txt   # Music information
├── Quakespasm.html        # HTML documentation
├── Quakespasm.txt         # Text documentation
└── README.md              # This file
```

## [DOCUMENTATION FILES]

### User Documentation (`user/`)
- `CARNIFEX_CREDITS.md` - Credits and acknowledgments
- `CARNIFEX_ENGINE_GUIDE.md` - Complete engine development guide
- `CARNIFEX_ENGINE_SUMMARY.md` - Engine summary and overview
- `CARNIFEX_GAME_TEMPLATE.md` - Game template structure guide

### Project Documentation
- `PROJECT_STRUCTURE.md` - Complete project structure overview

### Development Documentation (`development/`)
- `AUDIO_FORMATS.md` - Audio format specifications
- `AUDIO_QUICK_REFERENCE.md` - Quick audio reference
- `MINIMAL_GAME_SUMMARY.md` - Minimal game implementation summary
- `TOOLS_README.md` - Tools documentation

### Music System Documentation
- **Enhanced Music Metadata System**: Automatic extraction and display of music metadata
- **Supported Formats**: OGG Vorbis comments, MP3 ID3v1 tags
- **Features**: Console logging, colored toaster notifications, menu configuration
- **Usage**: Console commands (`music track02`), automatic playback, configurable display

### Localization (`localization/`)
- `loc_english.txt` - English localization strings

## [PROJECT STRUCTURE]

The Carnifex project is organized as follows:

```
carnifex/
├── core/                  # Engine core files (formerly Quake/)
├── platforms/             # Platform-specific files
│   ├── Windows/          # Windows build files and libraries
│   ├── Linux/            # Linux build files and tools
│   └── Misc/             # Miscellaneous platform utilities
├── tools/                 # CLI tools and utilities
├── carnifex-game/         # Game-specific assets
├── docs/                  # Documentation (this directory)
└── build-artifacts/       # Build output
```

## [QUICK START]

1. **Build the engine:**
   ```bash
   make build
   ```

2. **Run the game:**
   ```bash
   ./build-artifacts/carnifex-engine -game carnifex-game
   ```

## [DOCUMENTATION ORGANIZATION]

All project documentation has been organized into logical categories to keep the project structure clean and make information easy to find.
