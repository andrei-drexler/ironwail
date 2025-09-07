# 🏗️ Carnifex Project Structure

## 📁 **Complete Directory Organization**

The Carnifex project has been reorganized for better maintainability and clarity:

```
carnifex/
├── core/                          # Engine core files (formerly Quake/)
│   ├── *.c, *.h                  # 177 engine source files
│   ├── Makefile                  # Core build files
│   └── *.sh                      # Build scripts
├── platforms/                     # Platform-specific files
│   ├── Windows/                  # Windows build support
│   │   ├── cmake-modules/        # CMake modules
│   │   ├── CodeBlocks/           # Code::Blocks project files
│   │   ├── codecs/               # Audio codec libraries
│   │   ├── curl/                 # HTTP library
│   │   ├── misc/                 # Miscellaneous Windows files
│   │   ├── SDL/                  # SDL 1.2 libraries
│   │   ├── SDL2/                 # SDL 2.0 libraries
│   │   ├── VisualStudio/         # Visual Studio project files
│   │   ├── zlib/                 # Compression library
│   │   ├── QuakeSpasm.ico        # Application icon
│   │   └── QuakeSpasm.rc         # Windows resource file
│   ├── Linux/                    # Linux build support
│   │   ├── CodeBlocks/           # Code::Blocks project files
│   │   └── sgml/                 # Documentation generation
│   └── Misc/                     # Miscellaneous platform files
│       ├── cmake/                # CMake utilities
│       ├── pak/                  # Default game assets
│       ├── *.txt                 # Documentation files
│       ├── *.patch               # Source patches
│       └── *.png                 # Icons and images
├── tools/                         # CLI tools and utilities
│   ├── build/                    # Build scripts
│   │   └── build_carnifex.sh     # Main build script
│   ├── lib/                      # Tool libraries
│   │   ├── lmp.c, lmp.h          # LMP file handling
│   │   └── pak.c, pak.h          # PAK file handling
│   ├── scripts/                  # Utility scripts
│   │   └── audio_examples.sh     # Audio examples
│   ├── carnifex-cli              # Command line interface
│   ├── carnifex-cli.c            # CLI source code
│   └── Makefile                  # Tools build file
├── carnifex-game/                 # Game-specific assets
│   ├── gfx/                      # Graphics assets
│   ├── music/                    # Music files
│   │   ├── track02.ogg           # Music track 2
│   │   └── track03.ogg           # Music track 3
│   ├── localization/             # Localization files
│   ├── pak0.pak                  # Game data package 0
│   ├── pak1.pak                  # Game data package 1
│   └── quake.rc                  # Game resource file
├── docs/                          # Documentation
│   ├── development/              # Development documentation
│   │   ├── AUDIO_FORMATS.md      # Audio format specifications
│   │   ├── AUDIO_QUICK_REFERENCE.md
│   │   ├── MINIMAL_GAME_SUMMARY.md
│   │   └── TOOLS_README.md       # Tools documentation
│   ├── localization/             # Documentation localization
│   │   └── loc_english.txt       # English strings
│   ├── user/                     # User documentation
│   │   ├── CARNIFEX_CREDITS.md   # Credits and acknowledgments
│   │   ├── CARNIFEX_ENGINE_GUIDE.md
│   │   ├── CARNIFEX_ENGINE_SUMMARY.md
│   │   └── CARNIFEX_GAME_TEMPLATE.md
│   ├── Quakespasm-Music.txt      # Music information
│   ├── Quakespasm.html           # HTML documentation
│   ├── Quakespasm.txt            # Text documentation
│   ├── README.md                 # Documentation index
│   └── PROJECT_STRUCTURE.md      # This file
├── build-artifacts/               # Build output directory
│   ├── carnifex-engine           # Compiled executable
│   ├── CMakeCache.txt            # CMake cache
│   ├── CMakeFiles/               # CMake build files
│   ├── cmake_install.cmake       # CMake install script
│   └── Makefile                  # Generated Makefile
├── CMakeLists.txt                 # Main CMake configuration
└── Makefile                       # Main project Makefile
```

## 🎯 **Directory Purposes**

### **`core/`** - Engine Core
- Contains all engine source code (formerly `Quake/`)
- 177 files including C source, headers, and build scripts
- Platform-independent engine logic
- Reusable across different games

### **`platforms/`** - Platform Support
- **`Windows/`** - Windows-specific build files, libraries, and tools
- **`Linux/`** - Linux-specific build files and tools
- **`Misc/`** - Cross-platform utilities and patches
- Contains platform-specific libraries (SDL, codecs, etc.)

### **`tools/`** - Development Tools
- **`build/`** - Build scripts and automation
- **`lib/`** - Tool libraries for file handling
- **`scripts/`** - Utility scripts
- CLI tools for game development

### **`carnifex-game/`** - Game Assets
- Game-specific content (NOT engine files)
- Graphics, music, localization files
- Game data packages (PAK files)
- Resource files

### **`docs/`** - Documentation
- **`development/`** - Technical documentation
- **`user/`** - User guides and tutorials
- **`localization/`** - Documentation localization
- Complete project documentation

### **`build-artifacts/`** - Build Output
- Compiled executable
- CMake build files
- Temporary build artifacts

## 🚀 **Build Process**

1. **Clean build:**
   ```bash
   make clean
   ```

2. **Build engine:**
   ```bash
   make build
   ```

3. **Run game:**
   ```bash
   ./build-artifacts/carnifex-engine -game carnifex-game
   ```

## 🔧 **Key Benefits of This Structure**

### **✅ Separation of Concerns**
- **Engine core** (`core/`) - Reusable engine code
- **Platform support** (`platforms/`) - OS-specific files
- **Game assets** (`carnifex-game/`) - Game-specific content
- **Development tools** (`tools/`) - CLI and utilities

### **✅ Maintainability**
- Clear directory purposes
- Easy to locate specific files
- Logical organization
- Scalable structure

### **✅ Development Workflow**
- Clean build system
- Platform-specific support
- Comprehensive documentation
- Tool integration

### **✅ Game Development**
- Clear separation of engine vs. game
- Easy asset management
- Flexible game structure
- Independent game development

## 📚 **Documentation Structure**

The documentation is organized into logical categories:

- **User Documentation** - Guides for end users and game developers
- **Development Documentation** - Technical specifications and references
- **Localization** - Multi-language support
- **Project Structure** - This comprehensive overview

## 🎮 **Game Development Workflow**

1. **Engine Development** - Work in `core/` directory
2. **Platform Support** - Add platform-specific code in `platforms/`
3. **Game Assets** - Create game content in `carnifex-game/`
4. **Tools** - Use CLI tools from `tools/` directory
5. **Documentation** - Update docs in `docs/` directory

This structure provides a solid foundation for both engine development and game creation, with clear separation between reusable engine code and game-specific assets.

---

**Carnifex Engine** - Building the future whilst honouring the past. 🎮
