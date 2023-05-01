#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "shared/log.h"

#define A2DP_BTTEST_ENCDEC(a) (strstr((a), "enc") != NULL ? "enc" : "dec")

FILE *a2dp_bttest_create(const char *codec_name, const char *encdec) {
    char fname[256];
    assert(strlen(codec_name) <= 64);

    snprintf(fname, sizeof(fname), "/home/user/bttest_files/%16s-%3s.bin", codec_name, A2DP_BTTEST_ENCDEC(encdec));

    FILE *f = fopen(fname, "wb");

    if (f == NULL) {
        error("Could not open file %s: %s", fname, strerror(ferror(f)));
    }

    return f;
}


void a2dp_bttest_write_frame(FILE *f, const uint8_t *frame, size_t len) {
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
    fclose(f);
}
