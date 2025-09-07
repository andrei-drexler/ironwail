#!/bin/bash
# Audio Preparation Examples for Carnifex Engine

echo "=== Carnifex Audio Preparation Examples ==="

# Example 1: Convert a single sound effect
echo "1. Converting a single sound effect:"
echo "./tools/carnifex-cli -A -i input.wav -o sound/misc/menu1.wav -T sound_effect"

# Example 2: Convert a music file
echo "2. Converting a music file:"
echo "./tools/carnifex-cli -A -i music_source.wav -o music/track01.wav -T music -F mp3"

# Example 3: Create Carnifex directory structure
echo "3. Creating Carnifex directory structure:"
echo "./tools/carnifex-cli -S -o carnifex/"

# Example 4: Validate existing files
echo "4. Validating existing audio files:"
echo "./tools/carnifex-cli -V -i carnifex/sound/"

# Example 5: Using FFmpeg directly
echo "5. Direct FFmpeg commands:"
echo "# Sound effect (mono, 44100 Hz, 16-bit):"
echo "ffmpeg -i input.wav -ac 1 -ar 44100 -c:a pcm_s16le output.wav"
echo ""
echo "# Music (stereo, 44100 Hz, 16-bit):"
echo "ffmpeg -i input.wav -ar 44100 -c:a pcm_s16le output.wav"
echo ""
echo "# Compressed music (MP3):"
echo "ffmpeg -i input.wav -ar 44100 -c:a libmp3lame -b:a 128k output.mp3"

# Example 6: Create PAK file
echo "6. Creating PAK file:"
echo "./tools/carnifex-cli -c -i carnifex/ -o carnifex.pak"

# Example 7: Test audio in engine
echo "7. Testing audio in engine:"
echo "echo 'play sound/misc/menu1.wav' | ./carnifex-engine -dev"

echo ""
echo "=== Common Issues and Solutions ==="
echo "Error: 'is a stereo sample'"
echo "Fix: Convert to mono with -ac 1"
echo ""
echo "Error: 'data extends beyond file size'"
echo "Fix: Convert to 44100 Hz with -ar 44100"
echo ""
echo "Error: 'has zero samples'"
echo "Fix: Recreate file with actual audio content"
echo ""
echo "Error: 'not 8 or 16 bit'"
echo "Fix: Convert bit depth with -c:a pcm_s16le"
