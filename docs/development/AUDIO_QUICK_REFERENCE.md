# Audio Quick Reference Guide

## Quick Commands

### Convert Audio for Carnifex Engine
```bash
# Sound Effects (mono, 44100 Hz, 16-bit)
ffmpeg -i input.wav -ac 1 -ar 44100 -c:a pcm_s16le output.wav

# Music (stereo, 44100 Hz, 16-bit)  
ffmpeg -i input.wav -ar 44100 -c:a pcm_s16le output.wav

# Compressed Music (MP3)
ffmpeg -i input.wav -ar 44100 -c:a libmp3lame -b:a 128k output.mp3
```

### Create PAK Files
```bash
# Create game PAK
./tools/carnifex-cli -c -i carnifex/ -o carnifex.pak

# Extract PAK
./tools/carnifex-cli -e -i carnifex.pak -o extracted/
```

### Audio Processing
```bash
# Convert to sound effect
./tools/carnifex-cli -A -i input.wav -o sound/menu1.wav -T sound_effect

# Convert to music
./tools/carnifex-cli -A -i music.wav -o music/track01.wav -T music -F mp3

# Validate audio files
./tools/carnifex-cli -V -i sound/

# Create directory structure
./tools/carnifex-cli -S -o carnifex/
```

## File Requirements

| Type | Format | Channels | Sample Rate | Bit Depth | Notes |
|------|--------|----------|-------------|-----------|-------|
| Sound Effects | WAV | Mono (1) | 44100 Hz | 8/16-bit | No metadata |
| Music | WAV/MP3/OGG | Stereo (2) | 44100 Hz | 16-bit | Streaming |

## Common Errors & Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `is a stereo sample` | Sound effect is stereo | Convert to mono: `-ac 1` |
| `data extends beyond file size` | Wrong sample rate | Convert to 44100 Hz: `-ar 44100` |
| `has zero samples` | Empty audio file | Recreate with actual audio |
| `not 8 or 16 bit` | Wrong bit depth | Convert: `-c:a pcm_s16le` |

## Directory Structure
```
carnifex/
├── sound/
│   ├── misc/          # menu1.wav, menu2.wav, menu3.wav
│   ├── weapons/       # ric1.wav, tink1.wav, etc.
│   ├── ambience/      # water1.wav, wind2.wav
│   └── [enemy]/       # hit.wav, etc.
└── music/             # track01.wav, track02.mp3
```

## Testing Commands
```bash
# Test sound in engine
echo "play sound/misc/menu1.wav" | ./carnifex-engine -dev

# List loaded sounds
echo "soundlist" | ./carnifex-engine -dev

# Check file format
file sound/misc/menu1.wav
```
