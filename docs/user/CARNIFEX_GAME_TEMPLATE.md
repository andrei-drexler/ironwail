# Carnifex Game Template

## 🎮 **Game Structure for Carnifex Engine**

This is a game template for the Carnifex Engine, based on IronWail.

## 📁 **Project Structure**

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
│   ├── maps/             # Game maps (.bsp)
│   ├── progs/            # Game logic (progs.dat)
│   ├── sound/            # Sound effects (.wav)
│   ├── gfx/              # Graphics and textures
│   ├── music/            # Music files
│   ├── localization/     # Localization files
│   ├── pak0.pak          # Game data package
│   ├── pak1.pak          # Game data package
│   └── quake.rc          # Resource file
├── docs/                  # Documentation
└── build-artifacts/       # Build output
```

## 🚀 **How to Use**

1. **Compile the engine:**
   ```bash
   cd /home/deck/development/carnifex
   make build
   ```

2. **Run your game:**
   ```bash
   ./build-artifacts/carnifex-engine -game carnifex-game
   ```

## 🎯 **Carnifex Engine Advantages**

- ✅ **No dependency on original Quake**
- ✅ **Optimised IronWail performance**
- ✅ **Support for independent games**
- ✅ **All original credits preserved**
- ✅ **Flexible directory structure**

## 📚 **Resources**

- **Maps**: Create maps with TrenchBroom or NetRadiant
- **Progs**: Develop logic in QuakeC
- **Sound**: Add sound effects
- **Gfx**: Include textures and graphics

## 🏆 **Credits**

Based on the IronWail Engine, which is based on QuakeSpasm, which is based on the original Quake by id Software.

All original credits have been preserved.

---

**Carnifex Engine** - Building the future whilst honouring the past.
