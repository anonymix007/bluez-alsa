#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "shared/log.h"

#define A2DP_BTTEST_ENCDEC(a) (strstr((a), "enc") != NULL ? "enc" : "dec")

static char *codec_names[] = {
    "ldac",
    "aac",
    "sbc",
    "aptx-hd",
    "aptx",
};

char *codec_name(const char *fname) {
    for (int i = 0; i < sizeof(codec_names)/sizeof(codec_names[0]); i++) {
        if (strstr(fname, codec_names[i]) != NULL) return codec_names[i];
    }
    return "unknown";
}

FILE *a2dp_bttest_create(const char *file_name, const char *encdec) {
    char fname[256];

    snprintf(fname, sizeof(fname), "/home/user/bttest_files/%s-%3s.bin", codec_name(file_name), A2DP_BTTEST_ENCDEC(encdec));

    FILE *f = fopen(fname, "wb");

    if (f == NULL) {
        error("Could not open file %s: %s", fname, strerror(errno));
    } else {
        debug("File %s opened successfully", fname);
    }

    return f;
}


void a2dp_bttest_write_frame(FILE *f, const uint8_t *frame, size_t len) {
    debug("Size: %zu, first byte %x", len, frame[0]);
    fwrite(&len, sizeof(len), 1, f);
    fwrite(frame, sizeof(frame[0]), len, f);
}

void a2dp_bttest_write_frames(FILE *f, const uint8_t *frames, size_t total_len, size_t count) {
    size_t frame_len = total_len / count;
    for (int i = 0; i < count; i++) {
        a2dp_bttest_write_frame(f, frames + i * frame_len, frame_len);
    }
    fflush(f);
}
void a2dp_bttest_close(FILE *f) {
    debug("File closed ");
    fclose(f);
}
