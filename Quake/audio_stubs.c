/*
 * Minimal audio library stubs for headless testing
 * Provides no-op implementations of Vorbis, FLAC, and MP3 libraries
 */

#include <stdio.h>
#include <stddef.h>

/* ============= Vorbisfile Stubs ============= */

typedef struct OggVorbis_File {
    void *datasource;
    int seekable;
    long offset;
    long end;
} OggVorbis_File;

typedef struct vorbis_info {
    int version;
    int channels;
    long rate;
    long bitrate_upper;
    long bitrate_nominal;
    long bitrate_lower;
    long bitrate_window;
    void *codec_setup;
} vorbis_info;

int ov_clear(OggVorbis_File *vf) {
    fprintf(stderr, "[Vorbis Stub] ov_clear() - stub\n");
    return 0;
}

int ov_open_callbacks(void *datasource, OggVorbis_File *vf,
                      const char *initial, long ibytes, void *callbacks) {
    fprintf(stderr, "[Vorbis Stub] ov_open_callbacks() - stub (returns error)\n");
    return -1;  // Error - no Vorbis support
}

long ov_read(OggVorbis_File *vf, char *buffer, int length,
             int bigendianp, int word, int sgned, int *bitstream) {
    return 0;  // No data
}

vorbis_info *ov_info(OggVorbis_File *vf, int link) {
    static vorbis_info info = {0};
    return &info;
}

long ov_pcm_total(OggVorbis_File *vf, int i) {
    return 0;
}

int ov_seekable(OggVorbis_File *vf) {
    return 0;
}

int ov_streams(OggVorbis_File *vf) {
    return 1;
}

int ov_time_seek(OggVorbis_File *vf, double pos) {
    return -1;
}

/* ============= FLAC Stubs ============= */

int FLAC__stream_decoder_init_file(void) {
    fprintf(stderr, "[FLAC Stub] FLAC__stream_decoder_init_file() - stub\n");
    return 0;
}

void FLAC__stream_decoder_delete(void *decoder) {
    fprintf(stderr, "[FLAC Stub] FLAC__stream_decoder_delete() - stub\n");
}

/* ============= MP3/mpg123 Stubs ============= */

typedef struct mpg123_handle_struct {
    int dummy;
} mpg123_handle;

int mpg123_init(void) {
    fprintf(stderr, "[MP3 Stub] mpg123_init() - stub\n");
    return 0;
}

mpg123_handle *mpg123_new(const char *decoder, int *error) {
    fprintf(stderr, "[MP3 Stub] mpg123_new() - stub (returns NULL)\n");
    if (error) *error = -1;
    return NULL;
}

void mpg123_delete(mpg123_handle *mh) {
    fprintf(stderr, "[MP3 Stub] mpg123_delete() - stub\n");
}

int mpg123_open(mpg123_handle *mh, const char *path) {
    return -1;  // Error
}

int mpg123_open_handle(mpg123_handle *mh, void *handle) {
    return -1;  // Error
}

int mpg123_close(mpg123_handle *mh) {
    return 0;
}

int mpg123_read(mpg123_handle *mh, unsigned char *outmemory,
                size_t outmemsize, size_t *done) {
    if (done) *done = 0;
    return -1;  // Error
}

int mpg123_format_none(mpg123_handle *mh) {
    return 0;
}

int mpg123_format(mpg123_handle *mh, long rate, int channels, int encodings) {
    return 0;
}

int mpg123_format_support(mpg123_handle *mh, long rate, int encoding) {
    return 0;
}

int mpg123_getformat(mpg123_handle *mh, long *rate, int *channels, int *encoding) {
    if (rate) *rate = 44100;
    if (channels) *channels = 2;
    if (encoding) *encoding = 0;
    return -1;  // Error
}

int mpg123_replace_reader_handle(mpg123_handle *mh, void *r_read, void *r_lseek, void *cleanup) {
    return 0;
}

long mpg123_seek(mpg123_handle *mh, long sampleoff, int whence) {
    return -1;
}

void mpg123_exit(void) {
    fprintf(stderr, "[MP3 Stub] mpg123_exit() - stub\n");
}

/* ============= Ogg Stubs ============= */

typedef struct ogg_sync_state {
    unsigned char *data;
    int storage;
    int fill;
    int returned;
    int unsynced;
    int headerbytes;
    int bodybytes;
} ogg_sync_state;

int ogg_sync_init(ogg_sync_state *oy) {
    fprintf(stderr, "[Ogg Stub] ogg_sync_init() - stub\n");
    return 0;
}

int ogg_sync_clear(ogg_sync_state *oy) {
    return 0;
}
