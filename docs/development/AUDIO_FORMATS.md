# Audio Formats and PAK Deployment Guide

## Overview

This document describes the audio format requirements for the Carnifex engine and how to properly prepare audio files for deployment in game PAK files.

## Supported Audio Formats

### Music (Background Music)
The engine supports multiple audio formats for background music:

- **WAV** - 8-bit or 16-bit, mono or stereo
- **MP3** - If libmpg123 is available
- **OGG/Vorbis** - If libvorbis is available  
- **FLAC** - If libflac is available
- **OPUS** - If libopus is available
- **MOD formats** - IT, S3M, XM, MOD
- **UMX** - Unreal Music files

**Music files can be stereo** and are played through the streaming audio system.

### Sound Effects
Sound effects have stricter requirements:

- **WAV only** - 8-bit or 16-bit
- **Mono only** - Stereo files are rejected
- **Sample rates**: 22050 Hz or 44100 Hz (recommended)

**Sound effects must be mono** and are loaded into memory for fast playback.

## Audio Format Specifications

### WAV File Requirements

#### For Sound Effects:
```
Format: WAV (Microsoft PCM)
Channels: 1 (mono only)
Sample Rate: 44100 Hz (recommended) or 22050 Hz
Bit Depth: 8-bit or 16-bit
Encoding: PCM (uncompressed)
Metadata: None (no LIST/INFO chunks)
```

#### For Music:
```
Format: WAV, MP3, OGG, FLAC, or OPUS
Channels: 1 (mono) or 2 (stereo)
Sample Rate: 44100 Hz (recommended)
Bit Depth: 16-bit (for WAV)
Encoding: PCM (WAV) or compressed (others)
```

## Common Issues and Solutions

### Issue 1: "is a stereo sample" Error
**Problem**: Sound effect files are stereo instead of mono.

**Solution**:
```bash
# Convert stereo to mono
ffmpeg -i input.wav -ac 1 -ar 44100 -c:a pcm_s16le output.wav
```

### Issue 2: "data extends beyond file size" Error
**Problem**: Sample rate mismatch causes resampling issues.

**Solution**:
```bash
# Convert to engine's sample rate (44100 Hz)
ffmpeg -i input.wav -ac 1 -ar 44100 -c:a pcm_s16le output.wav
```

### Issue 3: "has zero samples" Error
**Problem**: WAV file has no audio data (empty data chunk).

**Solution**: Create a proper WAV file with actual audio content.

### Issue 4: Metadata Chunks
**Problem**: WAV files contain LIST/INFO metadata chunks that confuse the parser.

**Solution**:
```bash
# Create clean WAV without metadata
ffmpeg -i input.wav -ac 1 -ar 44100 -c:a pcm_s16le -y output.wav
```

## File Preparation Workflow

### Step 1: Audio Source Preparation
1. **Source Quality**: Use high-quality source audio (44.1kHz, 16-bit minimum)
2. **Normalization**: Normalize audio levels to prevent clipping
3. **Editing**: Trim silence, apply any necessary effects

### Step 2: Format Conversion

#### For Sound Effects:
```bash
# Convert to mono WAV at 44100 Hz
ffmpeg -i source.wav -ac 1 -ar 44100 -c:a pcm_s16le -y sound_effect.wav

# For 8-bit sound effects (smaller file size)
ffmpeg -i source.wav -ac 1 -ar 44100 -c:a pcm_u8 -y sound_effect_8bit.wav
```

#### For Music:
```bash
# High quality WAV (recommended)
ffmpeg -i source.wav -ar 44100 -c:a pcm_s16le -y music.wav

# Compressed MP3 (smaller file size)
ffmpeg -i source.wav -ar 44100 -c:a libmp3lame -b:a 128k -y music.mp3

# Compressed OGG (good quality/size ratio)
ffmpeg -i source.wav -ar 44100 -c:a libvorbis -q:a 4 -y music.ogg
```

### Step 3: Validation
```bash
# Check file format
file audio_file.wav

# Verify no metadata chunks
hexdump -C audio_file.wav | head -5

# Test in engine
echo "play audio_file.wav" | ./carnifex-engine -dev
```

## PAK File Deployment

### Directory Structure
```
carnifex/
├── sound/
│   ├── misc/           # Menu sounds, UI sounds
│   ├── weapons/        # Weapon sounds
│   ├── ambience/       # Environmental sounds
│   ├── hknight/        # Enemy sounds
│   ├── wizard/         # Enemy sounds
│   └── ...
├── music/              # Background music
│   ├── track01.wav
│   ├── track02.mp3
│   └── ...
└── ...
```

### Creating PAK Files
```bash
# Using carnifex-cli tool
./tools/carnifex-cli -c -i carnifex/ -o carnifex.pak

# Include specific directories
./tools/carnifex-cli -c -i carnifex/sound/ -o sound.pak
./tools/carnifex-cli -c -i carnifex/music/ -o music.pak
```

### PAK File Structure
```
carnifex.pak
├── sound/misc/menu1.wav
├── sound/misc/menu2.wav
├── sound/weapons/ric1.wav
├── music/track01.wav
└── ...
```

## Best Practices

### File Naming
- Use lowercase names with underscores
- Be descriptive: `weapon_shotgun_fire.wav`
- Avoid spaces and special characters
- Keep names under 32 characters

### File Organization
- Group related sounds in subdirectories
- Use consistent naming conventions
- Separate music from sound effects
- Keep file sizes reasonable

### Quality vs Size
- **Sound Effects**: 8-bit mono for simple sounds, 16-bit for complex ones
- **Music**: 16-bit stereo WAV for quality, MP3/OGG for size
- **Sample Rate**: 44100 Hz for compatibility, 22050 Hz for smaller files

### Testing
- Test all audio files in the engine before deployment
- Verify no console errors during loading
- Check audio quality and timing
- Test on different hardware configurations

## Troubleshooting

### Common Console Errors
```
misc/menu1.wav is a stereo sample
→ Convert to mono: ffmpeg -i input.wav -ac 1 output.wav

misc/menu1.wav data extends beyond file size  
→ Convert sample rate: ffmpeg -i input.wav -ar 44100 output.wav

misc/menu1.wav has zero samples
→ File has no audio data, recreate with proper content

misc/menu1.wav is not 8 or 16 bit
→ Convert bit depth: ffmpeg -i input.wav -c:a pcm_s16le output.wav
```

### Debug Commands
```bash
# List loaded sounds
soundlist

# Play specific sound
play sound/misc/menu1.wav

# Sound information
soundinfo

# Check music status
music
```

## Tools and Utilities

### FFmpeg Commands
```bash
# Convert any audio to engine-compatible WAV
ffmpeg -i input.any -ac 1 -ar 44100 -c:a pcm_s16le output.wav

# Create silent WAV for placeholders
ffmpeg -f lavfi -i "anullsrc=channel_layout=mono:sample_rate=44100" -t 0.1 -c:a pcm_s16le silent.wav

# Batch convert directory
for file in *.wav; do
    ffmpeg -i "$file" -ac 1 -ar 44100 -c:a pcm_s16le "converted_$file"
done
```

### Carnifex CLI Tool
```bash
# Create PAK file
./tools/carnifex-cli -c -i carnifex/ -o carnifex.pak

# Extract PAK file
./tools/carnifex-cli -e -i carnifex.pak -o extracted/

# List PAK contents
./tools/carnifex-cli -l -i carnifex.pak

# Convert audio file to sound effect
./tools/carnifex-cli -A -i input.wav -o sound/menu1.wav -T sound_effect

# Convert audio file to music
./tools/carnifex-cli -A -i music.wav -o music/track01.wav -T music -F mp3

# Validate audio files
./tools/carnifex-cli -V -i sound/

# Create directory structure
./tools/carnifex-cli -S -o carnifex/
```

## Performance Considerations

### Memory Usage
- Sound effects are loaded into memory
- Keep individual sound files under 1MB when possible
- Use 8-bit for simple sounds to save memory
- Compress music files to reduce PAK size

### Loading Times
- Minimize number of audio files
- Use appropriate compression
- Test loading times on target hardware
- Consider streaming for large music files

### Audio Quality
- Balance quality vs file size
- Test on different audio hardware
- Ensure consistent volume levels
- Use appropriate sample rates for content

## Conclusion

Proper audio preparation is crucial for a good gaming experience. Follow these guidelines to ensure compatibility, performance, and quality. Always test audio files in the engine before final deployment.

For questions or issues, refer to the engine console output and this documentation for troubleshooting steps.
