# Carnifex Engine

A modern Quake engine based on IronWail, designed for independent game development with enhanced stability and graceful error handling.

## 🎮 Features

- **Graceful Shutdown**: Robust error handling prevents crashes and provides clear error messages
- **Enhanced Stability**: Comprehensive memory protection and file validation
- **Cross-Platform**: Supports Linux, Windows, and other platforms
- **Independent Game Mode**: Designed for custom game development without requiring original Quake assets
- **Modern Graphics**: OpenGL rendering with advanced features
- **Sound System**: Enhanced audio handling with corruption detection

## 🚀 Quick Start

### Building the Engine

```bash
# Build the engine
make

# Or use the build script
./tools/build/build_carnifex.sh
```

### Running the Engine

```bash
# Run with default game directory
./carnifex-engine

# Run with specific game directory
./carnifex-engine -game your-game-directory

# Run with specific base directory
./carnifex-engine -basedir /path/to/your/quake/installation
```

## 📁 Project Structure

```
carnifex/
├── carnifex-engine          # Main engine executable
├── carnifex/                # Default game directory
│   ├── gfx.wad             # Graphics WAD file
│   ├── maps/               # Map files
│   ├── progs/              # Game logic
│   └── sound/              # Audio files
├── docs/                   # Documentation
│   ├── user/               # User documentation
│   └── development/        # Development documentation
├── tools/                  # Development tools
│   ├── build/              # Build scripts
│   └── scripts/            # Utility scripts
├── Quake/                  # Engine source code
└── README.md               # This file
```

## 🛠️ Development Tools

The `tools/scripts/` directory contains utility scripts for game development:

- `create_basic_progs.py` - Create basic game logic files
- `create_placeholder_graphics.py` - Generate placeholder graphics
- `create_simple_map.py` - Create simple test maps
- `fix_bsp.py` - Fix BSP map files

## 📚 Documentation

- **User Guide**: `docs/user/CARNIFEX_ENGINE_GUIDE.md`
- **Development Guide**: `docs/development/`
- **Engine Summary**: `docs/user/CARNIFEX_ENGINE_SUMMARY.md`

## 🔧 Recent Improvements

### Graceful Shutdown Implementation
- Added signal handlers for SIGSEGV, SIGTERM, SIGINT, and SIGFPE
- Implemented proper error handling for corrupted WAV files
- Fixed segmentation faults in file operations
- Enhanced memory safety with bounds checking

### Error Handling
- Clear error messages instead of cryptic crashes
- Proper cleanup on shutdown
- Robust file validation
- Cross-platform compatibility

## 🎯 Command Line Options

- `-game <directory>` - Specify game directory
- `-basedir <path>` - Set base directory
- `-rogue` - Load Rogue mission pack
- `-hipnotic` - Load Hipnotic mission pack
- `-quoth` - Load Quoth mission pack

## 🐛 Troubleshooting

If you encounter issues:

1. Check that your game directory contains the necessary files
2. Ensure WAD files are not corrupted
3. Verify file permissions
4. Check the console output for specific error messages

## 📄 License

This project is based on IronWail and follows the same licensing terms. See the original IronWail documentation for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For issues and questions, please check the documentation in the `docs/` directory or create an issue in the project repository.