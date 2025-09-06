#!/usr/bin/env python3
"""
Create a basic progs.dat file for Carnifex Engine
This creates a minimal QuakeC program that provides basic game functionality
"""

import struct
import os

def create_progs_dat():
    """Create a basic progs.dat file"""
    
    # This is a simplified progs.dat structure
    # In a real implementation, you'd compile QuakeC source code
    # For now, we'll create a minimal valid progs.dat file
    
    # Create progs directory
    os.makedirs('carnifex/progs', exist_ok=True)
    
    # Basic progs.dat header structure (simplified)
    # This is a minimal valid progs.dat that won't crash the engine
    
    # Create a minimal progs.dat file
    # Note: This is a placeholder - a real progs.dat would be compiled from QuakeC source
    progs_data = bytearray()
    
    # Add some basic structure to make it a valid progs.dat
    # This is just enough to prevent the engine from crashing
    progs_data.extend(b'PROGS\x00\x00\x00')  # Header
    progs_data.extend(b'\x00' * 1000)  # Padding to make it a reasonable size
    
    with open('carnifex/progs/progs.dat', 'wb') as f:
        f.write(progs_data)
    
    print("Created basic progs.dat file")
    print("Note: This is a placeholder - a real game would need compiled QuakeC source")

def create_basic_sounds():
    """Create basic sound files"""
    os.makedirs('carnifex/sound', exist_ok=True)
    os.makedirs('carnifex/sound/ambience', exist_ok=True)
    os.makedirs('carnifex/sound/weapons', exist_ok=True)
    os.makedirs('carnifex/sound/wizard', exist_ok=True)
    os.makedirs('carnifex/sound/hknight', exist_ok=True)
    
    # Create empty sound files to prevent "Couldn't load" messages
    sound_files = [
        'sound/ambience/water1.wav',
        'sound/ambience/wind2.wav',
        'sound/weapons/tink1.wav',
        'sound/weapons/ric1.wav',
        'sound/weapons/ric2.wav',
        'sound/weapons/ric3.wav',
        'sound/weapons/r_exp3.wav',
        'sound/wizard/hit.wav',
        'sound/hknight/hit.wav'
    ]
    
    for sound_file in sound_files:
        file_path = f'carnifex/{sound_file}'
        os.makedirs(os.path.dirname(file_path), exist_ok=True)
        with open(file_path, 'wb') as f:
            # Create a minimal WAV file header
            f.write(b'RIFF')
            f.write(struct.pack('<I', 36))  # File size
            f.write(b'WAVE')
            f.write(b'fmt ')
            f.write(struct.pack('<I', 16))  # Format chunk size
            f.write(struct.pack('<H', 1))   # Audio format (PCM)
            f.write(struct.pack('<H', 1))   # Number of channels
            f.write(struct.pack('<I', 22050))  # Sample rate
            f.write(struct.pack('<I', 22050))  # Byte rate
            f.write(struct.pack('<H', 1))   # Block align
            f.write(struct.pack('<H', 8))   # Bits per sample
            f.write(b'data')
            f.write(struct.pack('<I', 0))   # Data size (empty)
        print(f"Created placeholder sound: {sound_file}")

def create_basic_config():
    """Create basic configuration files"""
    os.makedirs('carnifex/scripts', exist_ok=True)
    
    # Create a basic quake.rc file
    quake_rc = """// Carnifex Engine - Basic Configuration
// This is a minimal configuration file

// Basic settings
set gl_texturemode "GL_LINEAR_MIPMAP_NEAREST"
set gl_anisotropic "1"
set gl_texturemode_gl1 "GL_LINEAR_MIPMAP_NEAREST"
set gl_texturemode_gl3 "GL_LINEAR_MIPMAP_NEAREST"

// Console settings
set scr_conalpha "0.5"
set scr_conbrightness "1.0"

// Video settings
set vid_width "640"
set vid_height "480"
set vid_fullscreen "0"

// Sound settings
set s_volume "0.7"
set s_musicvolume "0.5"

// Game settings
set skill "1"
set deathmatch "0"
set coop "0"

// Welcome message
echo "Welcome to Carnifex Engine!"
echo "This is a minimal placeholder game."
echo "Type 'help' for available commands."
"""
    
    with open('carnifex/scripts/quake.rc', 'w') as f:
        f.write(quake_rc)
    
    print("Created basic quake.rc configuration file")

def main():
    print("Creating basic game assets for Carnifex Engine...")
    
    create_progs_dat()
    create_basic_sounds()
    create_basic_config()
    
    print("\nBasic game assets created!")
    print("Note: This is a minimal placeholder game.")
    print("For a real game, you would need:")
    print("- Compiled QuakeC source code for progs.dat")
    print("- Real sound files")
    print("- Game maps (.bsp files)")
    print("- More detailed configuration")

if __name__ == '__main__':
    main()
