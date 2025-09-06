# Carnifex Engine

## 🎮 **Carnifex Engine v1.0.0-engine**

**Based on IronWail Engine** - A high-performance Quake engine fork modified for independent game development.

## What's this?

Carnifex Engine is a fork of the popular GLQuake descendant [IronWail](https://github.com/andrei-drexler/ironwail) (which is based on [QuakeSpasm](https://sourceforge.net/projects/quakespasm/)) with a focus on **independent game development** without requiring the original Quake game files.

## Key Features

### 🚀 **Independent Game Development**
- **No dependency on original Quake files** - Create and run games without needing Quake original
- **Flexible directory structure** - Support for custom game layouts
- **Removed shareware restrictions** - Full freedom for game development

### ⚡ **High Performance**
- All IronWail performance optimizations preserved
- GPU-accelerated rendering with modern OpenGL features
- Support for complex maps with high poly counts
- Decoupled renderer for smooth gameplay

### 🎯 **Developer Friendly**
- **Carnifex Engine branding** - Professional engine identity
- **Preserved original credits** - All IronWail and QuakeSpasm credits maintained
- **Flexible game structure** - Support for custom game directories
- **Modern build system** - Easy compilation and deployment

## System Requirements

| | Minimum GPU | Recommended GPU |
|:--|:--|:--|
|NVIDIA|GeForce GT 420 ("Fermi" 2010)|GeForce GT 630 or newer ("Kepler" 2012)|
|AMD|Radeon HD 5450 ("TeraScale 2" 2009) |Radeon HD 7700 series or newer ("GCN" 2012)|
|Intel|HD Graphics 4200 ("Haswell" 2012)|HD Graphics 620 ("Kaby Lake" 2016) or newer|

**Note**: Mac OS is not supported due to OpenGL 4.3 requirements (Apple deprecated OpenGL after 4.1).

## Quick Start

### 1. Build the Engine
```bash
./build_carnifex.sh
```

### 2. Run a Game
```bash
./carnifex-engine -game carnifex
```

### 3. Game Structure
```
carnifex/
├── maps/           # Game maps (.bsp)
├── progs/          # Game logic (progs.dat)
├── sound/          # Sound effects (.wav)
├── gfx/            # Graphics and textures
└── scripts/        # Configuration scripts
```

## Development

### Creating Games
1. **Maps**: Use TrenchBroom or NetRadiant to create levels
2. **Logic**: Develop game logic in QuakeC
3. **Assets**: Add sounds, textures, and models
4. **Testing**: Use the Carnifex Engine for testing

### Build System
- **CMake-based** build system
- **Cross-platform** support (Linux, Windows)
- **Automated scripts** for easy compilation

## Credits and Acknowledgments

### Original Developers
- **IronWail Team** - High-performance Quake engine
- **QuakeSpasm Team** - Modern GLQuake descendant
- **id Software** - Original Quake engine creators

### Carnifex Engine Modifications
- **Carnifex Engine Team** - Independent game development modifications
- All original credits and acknowledgments preserved
- Modifications made under GNU General Public License v2

## License

This engine is based on IronWail, which is based on QuakeSpasm, which is based on the original Quake engine.

- **Quake Engine**: Copyright (C) 1996-2001 Id Software, Inc.
- **QuakeSpasm**: GNU General Public License v2
- **IronWail**: GNU General Public License v2
- **Carnifex Engine**: GNU General Public License v2

## Documentation

- **[Carnifex Engine Guide](CARNIFEX_ENGINE_GUIDE.md)** - Complete development guide
- **[Credits](CARNIFEX_CREDITS.md)** - Detailed credits and acknowledgments
- **[Summary](CARNIFEX_ENGINE_SUMMARY.md)** - Project summary

## Purpose

The Carnifex Engine was created to:
- Enable independent game development without requiring original Quake files
- Maintain the performance and features of IronWail
- Preserve all original developer credits and acknowledgments
- Provide a solid foundation for new games

---

**Carnifex Engine** - Building the future while honoring the past.

*This engine is a modification of IronWail, created with respect for the original developers and their contributions to the gaming community.*