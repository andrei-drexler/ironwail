# Carnifex Development Tools

This directory contains development tools and scripts for the Carnifex engine.

## 📁 Directory Structure

- `build/` - Build scripts and configuration
- `scripts/` - Utility scripts for game development

## 🛠️ Build Tools

### `build/build_carnifex.sh`
Main build script for the Carnifex engine. Handles compilation, configuration, and linking.

**Usage:**
```bash
./tools/build/build_carnifex.sh
```

## 🎮 Game Development Scripts

### `scripts/create_basic_progs.py`
Creates basic game logic files (progs.dat) for testing and development.

**Usage:**
```bash
python3 tools/scripts/create_basic_progs.py
```

### `scripts/create_placeholder_graphics.py`
Generates placeholder graphics and WAD files for testing.

**Usage:**
```bash
python3 tools/scripts/create_placeholder_graphics.py
```

### `scripts/create_simple_map.py`
Creates simple test maps for development and testing.

**Usage:**
```bash
python3 tools/scripts/create_simple_map.py
```

### `scripts/fix_bsp.py`
Fixes BSP map files that may have issues.

**Usage:**
```bash
python3 tools/scripts/fix_bsp.py <mapfile.bsp>
```

## 📝 Notes

- All scripts require Python 3
- Make sure you have the necessary dependencies installed
- Scripts are designed to work with the Carnifex engine structure
- Always backup your files before running fix scripts

## 🔧 Requirements

- Python 3.6+
- Access to Carnifex engine source code
- Basic understanding of Quake file formats