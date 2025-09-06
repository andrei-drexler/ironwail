#!/usr/bin/env python3
"""
Fix stub graphics to have proper sizes instead of all being 8x8
This creates properly sized LMP files for different types of graphics
"""

import struct
import os

def create_lmp_file(filename, width, height, data):
    """Create an LMP file with the given dimensions and data"""
    # LMP format: width (4 bytes), height (4 bytes), data (width*height bytes)
    with open(filename, 'wb') as f:
        f.write(struct.pack('<I', width))  # width (little-endian)
        f.write(struct.pack('<I', height)) # height (little-endian)
        f.write(data)

def create_solid_color(width, height, color):
    """Create a solid color image"""
    return bytes([color] * (width * height))

def create_checkerboard(width, height, color1=252, color2=0):
    """Create a checkerboard pattern"""
    data = bytearray()
    for y in range(height):
        for x in range(width):
            if (x + y) % 2 == 0:
                data.append(color1)
            else:
                data.append(color2)
    return bytes(data)

def create_number_pattern(width, height, number):
    """Create a simple number pattern"""
    data = bytearray([0] * (width * height))
    
    # Simple 8x8 number patterns
    patterns = {
        0: [0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C],
        1: [0x08, 0x18, 0x28, 0x08, 0x08, 0x08, 0x08, 0x3E],
        2: [0x3C, 0x42, 0x02, 0x04, 0x08, 0x10, 0x20, 0x7E],
        3: [0x3C, 0x42, 0x02, 0x1C, 0x02, 0x02, 0x42, 0x3C],
        4: [0x04, 0x0C, 0x14, 0x24, 0x44, 0x7E, 0x04, 0x04],
        5: [0x7E, 0x40, 0x40, 0x7C, 0x02, 0x02, 0x42, 0x3C],
        6: [0x3C, 0x42, 0x40, 0x7C, 0x42, 0x42, 0x42, 0x3C],
        7: [0x7E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x20, 0x20],
        8: [0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x42, 0x3C],
        9: [0x3C, 0x42, 0x42, 0x42, 0x3E, 0x02, 0x42, 0x3C],
        'minus': [0x00, 0x00, 0x00, 0x7E, 0x7E, 0x00, 0x00, 0x00],
        'colon': [0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00],
        'slash': [0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00]
    }
    
    if number in patterns:
        pattern = patterns[number]
        # Center the pattern in the image
        start_x = (width - 8) // 2
        start_y = (height - 8) // 2
        
        for y in range(min(8, height - start_y)):
            for x in range(min(8, width - start_x)):
                if pattern[y] & (0x80 >> x):
                    data[(start_y + y) * width + (start_x + x)] = 255  # White
                else:
                    data[(start_y + y) * width + (start_x + x)] = 0    # Black
    
    return bytes(data)

def create_weapon_icon(width, height, weapon_type):
    """Create a simple weapon icon"""
    data = bytearray([0] * (width * height))
    
    # Simple weapon patterns
    if weapon_type == 'shotgun':
        # Simple shotgun shape
        for y in range(height):
            for x in range(width):
                if (x >= 2 and x <= 5 and y >= 2 and y <= 5):
                    data[y * width + x] = 255
    elif weapon_type == 'nailgun':
        # Simple nailgun shape
        for y in range(height):
            for x in range(width):
                if (x >= 1 and x <= 6 and y >= 3 and y <= 4):
                    data[y * width + x] = 255
    elif weapon_type == 'rlaunch':
        # Simple rocket launcher shape
        for y in range(height):
            for x in range(width):
                if (x >= 2 and x <= 5 and y >= 1 and y <= 6):
                    data[y * width + x] = 255
    elif weapon_type == 'lightng':
        # Simple lightning gun shape
        for y in range(height):
            for x in range(width):
                if (x >= 3 and x <= 4 and y >= 1 and y <= 6):
                    data[y * width + x] = 255
    
    return bytes(data)

def create_face_icon(width, height, face_type):
    """Create a simple face icon"""
    data = bytearray([0] * (width * height))
    
    # Simple face pattern
    for y in range(height):
        for x in range(width):
            # Face outline
            if (x == 1 or x == width-2) and (y >= 1 and y <= height-2):
                data[y * width + x] = 255
            elif (y == 1 or y == height-2) and (x >= 1 and x <= width-2):
                data[y * width + x] = 255
            # Eyes
            elif (x == 2 or x == width-3) and (y == 3):
                data[y * width + x] = 255
            # Mouth
            elif (x >= 2 and x <= width-3) and (y == height-3):
                data[y * width + x] = 255
    
    return bytes(data)

def main():
    # Create gfx directory if it doesn't exist
    os.makedirs('carnifex/gfx', exist_ok=True)
    
    print("Creating properly sized stub graphics for Carnifex Engine...")
    
    # Create number graphics (0-9) - 8x8 pixels
    for i in range(10):
        data = create_number_pattern(8, 8, i)
        create_lmp_file(f'carnifex/gfx/num_{i}.lmp', 8, 8, data)
        create_lmp_file(f'carnifex/gfx/anum_{i}.lmp', 8, 8, data)
        print(f"Created num_{i}.lmp and anum_{i}.lmp (8x8)")
    
    # Create special number graphics - 8x8 pixels
    create_lmp_file('carnifex/gfx/num_minus.lmp', 8, 8, create_number_pattern(8, 8, 'minus'))
    create_lmp_file('carnifex/gfx/anum_minus.lmp', 8, 8, create_number_pattern(8, 8, 'minus'))
    create_lmp_file('carnifex/gfx/num_colon.lmp', 8, 8, create_number_pattern(8, 8, 'colon'))
    create_lmp_file('carnifex/gfx/num_slash.lmp', 8, 8, create_number_pattern(8, 8, 'slash'))
    print("Created special number graphics (8x8)")
    
    # Create weapon graphics - 8x8 pixels
    weapons = ['shotgun', 'sshotgun', 'nailgun', 'snailgun', 'rlaunch', 'srlaunch', 'lightng']
    for weapon in weapons:
        data = create_weapon_icon(8, 8, weapon)
        create_lmp_file(f'carnifex/gfx/inv_{weapon}.lmp', 8, 8, data)
        create_lmp_file(f'carnifex/gfx/inv2_{weapon}.lmp', 8, 8, data)
        
        # Create alternative weapon graphics
        for i in range(1, 6):
            create_lmp_file(f'carnifex/gfx/inva{i}_{weapon}.lmp', 8, 8, data)
        print(f"Created weapon graphics for {weapon} (8x8)")
    
    # Create ammo graphics - 8x8 pixels
    ammo_types = ['shells', 'nails', 'rocket', 'cells']
    for ammo in ammo_types:
        data = create_checkerboard(8, 8, 255, 0)
        create_lmp_file(f'carnifex/gfx/sb_{ammo}.lmp', 8, 8, data)
        print(f"Created ammo graphic for {ammo} (8x8)")
    
    # Create armor graphics - 8x8 pixels
    for i in range(1, 4):
        data = create_solid_color(8, 8, 200 + i * 10)
        create_lmp_file(f'carnifex/gfx/sb_armor{i}.lmp', 8, 8, data)
        print(f"Created armor graphic {i} (8x8)")
    
    # Create item graphics - 8x8 pixels
    items = ['key1', 'key2', 'invis', 'invuln', 'suit', 'quad']
    for item in items:
        data = create_checkerboard(8, 8, 100, 200)
        create_lmp_file(f'carnifex/gfx/sb_{item}.lmp', 8, 8, data)
        print(f"Created item graphic for {item} (8x8)")
    
    # Create sigil graphics - 8x8 pixels
    for i in range(1, 5):
        data = create_solid_color(8, 8, 150 + i * 20)
        create_lmp_file(f'carnifex/gfx/sb_sigil{i}.lmp', 8, 8, data)
        print(f"Created sigil graphic {i} (8x8)")
    
    # Create face graphics - 8x8 pixels
    for i in range(1, 6):
        data = create_face_icon(8, 8, f'face{i}')
        create_lmp_file(f'carnifex/gfx/face{i}.lmp', 8, 8, data)
        create_lmp_file(f'carnifex/gfx/face_p{i}.lmp', 8, 8, data)
        print(f"Created face graphics {i} (8x8)")
    
    # Create special face graphics - 8x8 pixels
    special_faces = ['invis', 'invul2', 'inv2', 'quad']
    for face in special_faces:
        data = create_face_icon(8, 8, face)
        create_lmp_file(f'carnifex/gfx/face_{face}.lmp', 8, 8, data)
        print(f"Created special face graphic {face} (8x8)")
    
    # Create status bar graphics - 320x24 pixels (much larger!)
    status_bars = ['sbar', 'ibar', 'scorebar']
    for bar in status_bars:
        data = create_solid_color(320, 24, 50)  # Dark gray status bar
        create_lmp_file(f'carnifex/gfx/{bar}.lmp', 320, 24, data)
        print(f"Created status bar graphic {bar} (320x24)")
    
    # Create other graphics - 8x8 pixels
    other_graphics = ['disc', 'backtile', 'net', 'turtle']
    for graphic in other_graphics:
        data = create_checkerboard(8, 8, 128, 64)
        create_lmp_file(f'carnifex/gfx/{graphic}.lmp', 8, 8, data)
        print(f"Created graphic {graphic} (8x8)")
    
    print(f"\nCreated properly sized graphics files!")
    print("Now testing with carnifex-cli...")

if __name__ == '__main__':
    main()
