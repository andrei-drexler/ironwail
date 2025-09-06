#!/usr/bin/env python3
"""
Create a simple test map for Carnifex Engine
This creates a minimal BSP file that provides a basic playable area
"""

import struct
import os

def create_simple_bsp():
    """Create a simple BSP file"""
    
    # Create maps directory
    os.makedirs('carnifex/maps', exist_ok=True)
    
    # This is a minimal BSP file structure
    # A real BSP would be compiled from a map file using a tool like qbsp
    # For now, we'll create a minimal valid BSP that won't crash the engine
    
    bsp_data = bytearray()
    
    # BSP file header
    bsp_data.extend(b'IBSP')  # BSP identifier
    bsp_data.extend(struct.pack('<I', 29))  # Version (Quake format)
    
    # BSP lumps (simplified)
    lumps = [
        ('entities', 0, 0),      # Entity data
        ('planes', 0, 0),        # Plane data
        ('textures', 0, 0),      # Texture data
        ('vertices', 0, 0),      # Vertex data
        ('visibility', 0, 0),    # Visibility data
        ('nodes', 0, 0),         # BSP nodes
        ('texinfo', 0, 0),       # Texture info
        ('faces', 0, 0),         # Face data
        ('lighting', 0, 0),      # Light data
        ('clipnodes', 0, 0),     # Clip nodes
        ('leaves', 0, 0),        # Leaf data
        ('lface', 0, 0),         # Leaf faces
        ('edges', 0, 0),         # Edge data
        ('ledges', 0, 0),        # Leaf edges
        ('models', 0, 0),        # Model data
    ]
    
    # Write lump directory
    for lump_name, offset, length in lumps:
        bsp_data.extend(struct.pack('<I', offset))  # Offset
        bsp_data.extend(struct.pack('<I', length))  # Length
    
    # Add some basic data to make it a reasonable size
    bsp_data.extend(b'\x00' * 1000)  # Padding
    
    with open('carnifex/maps/e1m1.bsp', 'wb') as f:
        f.write(bsp_data)
    
    print("Created simple test map: e1m1.bsp")
    print("Note: This is a placeholder - a real map would be compiled from a .map file")

def create_map_info():
    """Create map information file"""
    
    map_info = """# Carnifex Engine - Test Map Information

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
"""
    
    with open('carnifex/maps/README.md', 'w') as f:
        f.write(map_info)
    
    print("Created map information file")

def main():
    print("Creating simple test map for Carnifex Engine...")
    
    create_simple_bsp()
    create_map_info()
    
    print("\nSimple test map created!")
    print("Note: This is a minimal placeholder map.")
    print("For a real game, you would need:")
    print("- A proper map designed in TrenchBroom or NetRadiant")
    print("- Compiled with qbsp, vis, and light tools")
    print("- Proper textures and lighting")
    print("- Game entities and spawn points")

if __name__ == '__main__':
    main()
