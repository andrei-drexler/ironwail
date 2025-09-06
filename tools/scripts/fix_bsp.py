#!/usr/bin/env python3
"""
Fix the BSP file to have a proper version number
"""

import struct

def fix_bsp_version():
    """Fix the BSP file version number"""
    
    with open('carnifex/maps/e1m1.bsp', 'rb') as f:
        data = bytearray(f.read())
    
    # Fix the version number (29 is the correct Quake BSP version)
    data[4:8] = struct.pack('<I', 29)
    
    with open('carnifex/maps/e1m1.bsp', 'wb') as f:
        f.write(data)
    
    # Copy to start.bsp as well
    with open('carnifex/maps/start.bsp', 'wb') as f:
        f.write(data)
    
    print("Fixed BSP version number")

if __name__ == '__main__':
    fix_bsp_version()
