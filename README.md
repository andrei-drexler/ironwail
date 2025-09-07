# Carnifex Engine

A modern, independent game engine based on IronWail, designed for creating original games without dependency on original Quake files.

## [OVERVIEW]

Carnifex Engine is a complete game development platform that provides:
- **Independent Game Development**: No dependency on original Quake files
- **Modern Features**: Enhanced graphics, audio, and user interface
- **Cross-Platform Support**: Linux, Windows, and other platforms
- **Extensible Architecture**: Easy to modify and extend for custom games

## [QUICK START]

### Building the Engine

```bash
# Clone the repository
git clone <repository-url>
cd carnifex

# Build the engine
make

# Run the engine
./build-artifacts/carnifex-engine -game carnifex-game
```

### Running the Engine

```bash
# Basic usage
./build-artifacts/carnifex-engine -game carnifex-game

# With development mode
./build-artifacts/carnifex-engine -game carnifex-game -dev

# Windowed mode
./build-artifacts/carnifex-engine -game carnifex-game -window
```

## [MUSIC SYSTEM]

### Enhanced Music Metadata System

The Carnifex Engine features a sophisticated music metadata system that automatically displays information about playing music.

#### Features

- **Automatic Metadata Extraction**: Supports OGG Vorbis comments and MP3 ID3v1 tags
- **Console Logging**: Displays `MUSIC: [Title] by [Artist]` in the console
- **Colored Toaster Notifications**: Beautiful on-screen display with:
  - **Title**: Dark white color
  - **Artist**: Light red color
  - **Duration**: 5 seconds (configurable)
- **Fallback Support**: Shows filename when no metadata is available
- **Menu Configuration**: Toggle console output and toaster display

#### Usage

1. **Console Commands**:
   ```
   music track02    # Play track02 with metadata display
   music track03    # Play track03 with metadata display
   ```

2. **Menu Options**:
   - Options → Game → Music Info Log (toggle console output)
   - Options → Game → Music Info Toaster (toggle on-screen display)

3. **Configuration Variables**:
   - `music_info_log` (default: 1) - Enable console logging
   - `music_info_toaster` (default: 1) - Enable toaster notifications
   - `music_info_toaster_duration` (default: 5) - Toaster display duration in seconds

#### Supported Audio Formats

- **OGG Vorbis** (.ogg) - Full metadata support
- **MP3** (.mp3) - ID3v1 tag support
- **WAV** (.wav) - Basic support
- **Module formats** (.it, .s3m, .xm, .mod, .umx) - Basic support

## [GRAPHICS FEATURES]

- **OpenGL Rendering**: Modern OpenGL-based graphics
- **High Resolution Support**: Up to 4K and beyond
- **Enhanced Textures**: Improved texture filtering and quality
- **Modern Shaders**: Advanced lighting and effects
- **Cross-Platform Compatibility**: Works on various graphics hardware

## [INPUT SYSTEM]

- **Gamepad Support**: Full controller support including Steam Deck
- **Gyro Controls**: Motion sensor support for compatible devices
- **Keyboard & Mouse**: Traditional PC input methods
- **Customizable Controls**: Configurable key bindings

## [DEVELOPMENT]

### Project Structure

```
carnifex/
├── core/                    # Engine core source code
│   ├── music_metadata.c     # Music metadata system
│   ├── music_metadata.h     # Music metadata headers
│   ├── bgmusic.c           # Background music system
│   ├── gl_screen.c         # Graphics rendering
│   ├── menu.c              # User interface
│   └── ...
├── carnifex-game/          # Game content
│   ├── music/              # Music files
│   ├── gfx/                # Graphics assets
│   └── ...
├── docs/                   # Documentation
├── tools/                  # Development tools
└── platforms/              # Platform-specific code
```

### Building from Source

#### Prerequisites

- **C Compiler**: GCC or Clang
- **CMake**: Version 3.10 or higher
- **OpenGL**: OpenGL 3.3 or higher
- **SDL2**: SDL2 development libraries
- **Audio Libraries**: Vorbis, XMP (optional: MP3, FLAC, Opus)

#### Linux Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libsdl2-dev libvorbis-dev libxmp-dev

# Arch Linux
sudo pacman -S base-devel cmake sdl2 libvorbis libxmp

# Fedora
sudo dnf install gcc cmake SDL2-devel libvorbis-devel libxmp-devel
```

#### Build Process

```bash
# Clean build
make clean
make

# Debug build
make DEBUG=1

# Release build
make RELEASE=1
```

### Adding Music with Metadata

To add music files with proper metadata:

1. **OGG Files**: Use `vorbiscomment` to add metadata:
   ```bash
   vorbiscomment -t "TITLE=Your Song Title" -t "ARTIST=Your Artist Name" -t "ALBUM=Your Album" -t "DATE=2025" your_file.ogg
   ```

2. **MP3 Files**: Use any MP3 tag editor to add ID3v1 tags

3. **Place Files**: Put music files in `carnifex-game/music/` directory

## [GAME DEVELOPMENT]

### Creating Custom Games

1. **Game Directory**: Create a new directory (e.g., `my-game/`)
2. **Content Structure**: Organize assets in subdirectories:
   ```
   my-game/
   ├── music/          # Music files
   ├── gfx/            # Graphics
   ├── maps/           # Level files
   └── ...
   ```
3. **Run Game**: `./build-artifacts/carnifex-engine -game my-game`

### Music Integration

The engine automatically:
- Scans for music files in the game's `music/` directory
- Extracts metadata from supported formats
- Displays information when music plays
- Supports both automatic and manual music playback

## [CONFIGURATION]

### Engine Configuration

- **Video Settings**: Resolution, fullscreen, vsync
- **Audio Settings**: Volume, audio device selection
- **Input Settings**: Key bindings, gamepad configuration
- **Game Settings**: Music info display, toaster notifications

### Configuration Files

- `carnifex.cfg` - Main configuration file
- `default.cfg` - Default settings
- `autoexec.cfg` - Auto-executed commands

## [TROUBLESHOOTING]

### Common Issues

1. **Build Errors**:
   - Ensure all dependencies are installed
   - Check CMake version (3.10+ required)
   - Verify OpenGL drivers are up to date

2. **Audio Issues**:
   - Check audio device configuration
   - Verify music files are in correct format
   - Ensure audio libraries are properly installed

3. **Graphics Issues**:
   - Update graphics drivers
   - Check OpenGL version compatibility
   - Verify display resolution settings

### Debug Mode

Run with `-dev` flag for additional debugging information:
```bash
./build-artifacts/carnifex-engine -game carnifex-game -dev
```

## [DOCUMENTATION]

- [Engine Guide](docs/user/CARNIFEX_ENGINE_GUIDE.md)
- [Engine Summary](docs/user/CARNIFEX_ENGINE_SUMMARY.md)
- [Game Template](docs/user/CARNIFEX_GAME_TEMPLATE.md)
- [Audio Formats](docs/development/AUDIO_FORMATS.md)
- [Tools Documentation](docs/development/TOOLS_README.md)

## [CONTRIBUTING]

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Code Style

- Follow existing code formatting
- Add comments for complex functionality
- Update documentation for new features
- Test on multiple platforms when possible

## [LICENSE]

This project is licensed under the GNU General Public License v2.0. See the LICENSE file for details.

## [CREDITS]

- **Based on IronWail**: Credits to original IronWail developers
- **Quake Engine**: Built upon the original Quake engine
- **Open Source Libraries**: Various open source audio and graphics libraries
- **Community**: Thanks to the Quake and game development community

## [MUSIC CREDITS]

- **Composer**: Edward 'Toy' Facundo
- **Album**: Carnifex (2025)
- **Tracks**: 
  - "Gallows Pole Awaits"
  - "Chant Of The Damned"

---

**Carnifex Engine** - Independent game development made simple.
