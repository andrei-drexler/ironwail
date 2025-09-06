# Carnifex Engine - Test Map Information

## Map: e1m1.bsp
- **Name**: Test Map
- **Type**: Single Player
- **Description**: A minimal test map for the Carnifex Engine
- **Size**: Small
- **Difficulty**: Easy

## Features
- Basic geometry
- Minimal lighting
- Placeholder textures
- No entities (yet)

## Notes
This is a placeholder map created for testing the Carnifex Engine.
A real game would need:
- Proper geometry designed in a map editor
- Compiled with qbsp, vis, and light tools
- Proper textures and lighting
- Game entities and spawn points

## How to Use
1. Run the Carnifex engine
2. Type 'map e1m1' in the console
3. The engine should load the map without crashing

## Development
To create a real map:
1. Use TrenchBroom or NetRadiant to design the map
2. Export as .map file
3. Compile with qbsp, vis, and light tools
4. Place the resulting .bsp file in carnifex/maps/
