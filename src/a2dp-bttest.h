#ifndef __A2DP_BTTEST_H__
#define __A2DP_BTTEST_H__
#include <stdio.h>
#include <stdint.h>

FILE *a2dp_bttest_create(const char *codec_name, const char *encdec);
void a2dp_bttest_write_frame(FILE *f, const uint8_t *frame, size_t len);
void a2dp_bttest_write_frames(FILE *f, const uint8_t *frames, size_t total_len, size_t count);
void a2dp_bttest_close(FILE *f);

#endif /* __A2DP_BTTEST_H__ */
