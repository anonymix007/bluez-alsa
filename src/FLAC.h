#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define DR_FLAC_NO_SIMD
#define DR_FLAC_NO_STDIO
#define DR_FLAC_NO_CRC
#define DR_FLAC_NO_OGG
#define DR_FLAC_BUFFER_SIZE 8
//#define DRFLAC_INLINE inline
#define DR_FLAC_MAX_BLOCK_SIZE 512
#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>

void *prealloc(void *ptr, size_t size) {
    return NULL;
}


#if 0
static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_left_side__reference_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples)
{
    drflac_uint64 i;
    for (i = 0; i < frameCount; ++i) {
        drflac_uint32 left  = (drflac_uint32)pInputSamples0[i] << (unusedBitsPerSample + pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample);
        drflac_uint32 side  = (drflac_uint32)pInputSamples1[i] << (unusedBitsPerSample + pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample);
        drflac_uint32 right = left - side;

        pOutputSamples[i*2+0] = (drflac_int32)left;
        pOutputSamples[i*2+1] = (drflac_int32)right;
    }
}
#endif

static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_left_side__scalar_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples_l, drflac_int32* pOutputSamples_r)
{
    drflac_uint64 i;
    drflac_uint64 frameCount4 = frameCount >> 2;
    const drflac_uint32* pInputSamples0U32 = (const drflac_uint32*)pInputSamples0;
    const drflac_uint32* pInputSamples1U32 = (const drflac_uint32*)pInputSamples1;
    drflac_uint32 shift0 = unusedBitsPerSample + pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
    drflac_uint32 shift1 = unusedBitsPerSample + pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;

    for (i = 0; i < frameCount4; ++i) {
        drflac_uint32 left0 = pInputSamples0U32[i*4+0] << shift0;
        drflac_uint32 left1 = pInputSamples0U32[i*4+1] << shift0;
        drflac_uint32 left2 = pInputSamples0U32[i*4+2] << shift0;
        drflac_uint32 left3 = pInputSamples0U32[i*4+3] << shift0;

        drflac_uint32 side0 = pInputSamples1U32[i*4+0] << shift1;
        drflac_uint32 side1 = pInputSamples1U32[i*4+1] << shift1;
        drflac_uint32 side2 = pInputSamples1U32[i*4+2] << shift1;
        drflac_uint32 side3 = pInputSamples1U32[i*4+3] << shift1;

        drflac_uint32 right0 = left0 - side0;
        drflac_uint32 right1 = left1 - side1;
        drflac_uint32 right2 = left2 - side2;
        drflac_uint32 right3 = left3 - side3;

        pOutputSamples_l[i*4+0] = (drflac_int32)left0;
        pOutputSamples_r[i*4+0] = (drflac_int32)right0;
        pOutputSamples_l[i*4+1] = (drflac_int32)left1;
        pOutputSamples_r[i*4+1] = (drflac_int32)right1;
        pOutputSamples_l[i*4+2] = (drflac_int32)left2;
        pOutputSamples_r[i*4+2] = (drflac_int32)right2;
        pOutputSamples_l[i*4+3] = (drflac_int32)left3;
        pOutputSamples_r[i*4+3] = (drflac_int32)right3;
    }

    for (i = (frameCount4 << 2); i < frameCount; ++i) {
        drflac_uint32 left  = pInputSamples0U32[i] << shift0;
        drflac_uint32 side  = pInputSamples1U32[i] << shift1;
        drflac_uint32 right = left - side;

        pOutputSamples_l[i] = (drflac_int32)left;
        pOutputSamples_r[i] = (drflac_int32)right;
    }
}


static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_left_side_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples_l, drflac_int32* pOutputSamples_r)
{
    {
        /* Scalar fallback. */
#if 0
        drflac_read_pcm_frames_s32__decode_left_side__reference_deinterleaved(pFlac, frameCount, unusedBitsPerSample, pInputSamples0, pInputSamples1, pOutputSamples);
#else
        drflac_read_pcm_frames_s32__decode_left_side__scalar_deinterleaved(pFlac, frameCount, unusedBitsPerSample, pInputSamples0, pInputSamples1, pOutputSamples_l, pOutputSamples_r);
#endif
    }
}


#if 0
static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_right_side__reference_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples)
{
    drflac_uint64 i;
    for (i = 0; i < frameCount; ++i) {
        drflac_uint32 side  = (drflac_uint32)pInputSamples0[i] << (unusedBitsPerSample + pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample);
        drflac_uint32 right = (drflac_uint32)pInputSamples1[i] << (unusedBitsPerSample + pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample);
        drflac_uint32 left  = right + side;

        pOutputSamples[i*2+0] = (drflac_int32)left;
        pOutputSamples[i*2+1] = (drflac_int32)right;
    }
}
#endif

static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_right_side__scalar_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples_l, drflac_int32* pOutputSamples_r)
{
    drflac_uint64 i;
    drflac_uint64 frameCount4 = frameCount >> 2;
    const drflac_uint32* pInputSamples0U32 = (const drflac_uint32*)pInputSamples0;
    const drflac_uint32* pInputSamples1U32 = (const drflac_uint32*)pInputSamples1;
    drflac_uint32 shift0 = unusedBitsPerSample + pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
    drflac_uint32 shift1 = unusedBitsPerSample + pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;

    for (i = 0; i < frameCount4; ++i) {
        drflac_uint32 side0  = pInputSamples0U32[i*4+0] << shift0;
        drflac_uint32 side1  = pInputSamples0U32[i*4+1] << shift0;
        drflac_uint32 side2  = pInputSamples0U32[i*4+2] << shift0;
        drflac_uint32 side3  = pInputSamples0U32[i*4+3] << shift0;

        drflac_uint32 right0 = pInputSamples1U32[i*4+0] << shift1;
        drflac_uint32 right1 = pInputSamples1U32[i*4+1] << shift1;
        drflac_uint32 right2 = pInputSamples1U32[i*4+2] << shift1;
        drflac_uint32 right3 = pInputSamples1U32[i*4+3] << shift1;

        drflac_uint32 left0 = right0 + side0;
        drflac_uint32 left1 = right1 + side1;
        drflac_uint32 left2 = right2 + side2;
        drflac_uint32 left3 = right3 + side3;

        pOutputSamples_l[i*4+0] = (drflac_int32)left0;
        pOutputSamples_r[i*4+0] = (drflac_int32)right0;
        pOutputSamples_l[i*4+1] = (drflac_int32)left1;
        pOutputSamples_r[i*4+1] = (drflac_int32)right1;
        pOutputSamples_l[i*4+2] = (drflac_int32)left2;
        pOutputSamples_r[i*4+2] = (drflac_int32)right2;
        pOutputSamples_l[i*4+3] = (drflac_int32)left3;
        pOutputSamples_r[i*4+3] = (drflac_int32)right3;
    }

    for (i = (frameCount4 << 2); i < frameCount; ++i) {
        drflac_uint32 side  = pInputSamples0U32[i] << shift0;
        drflac_uint32 right = pInputSamples1U32[i] << shift1;
        drflac_uint32 left  = right + side;

        pOutputSamples_l[i] = (drflac_int32)left;
        pOutputSamples_r[i] = (drflac_int32)right;
    }
}

static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_right_side_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples_l, drflac_int32* pOutputSamples_r)
{
    {
        /* Scalar fallback. */
#if 0
        drflac_read_pcm_frames_s32__decode_right_side__reference_deinterleaved(pFlac, frameCount, unusedBitsPerSample, pInputSamples0, pInputSamples1, pOutputSamples);
#else
        drflac_read_pcm_frames_s32__decode_right_side__scalar_deinterleaved(pFlac, frameCount, unusedBitsPerSample, pInputSamples0, pInputSamples1, pOutputSamples_l, pOutputSamples_r);
#endif
    }
}

#if 0
static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_mid_side__reference_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples)
{
    for (drflac_uint64 i = 0; i < frameCount; ++i) {
        drflac_uint32 mid  = pInputSamples0U32[i] << pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
        drflac_uint32 side = pInputSamples1U32[i] << pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;

        mid = (mid << 1) | (side & 0x01);

        pOutputSamples[i*2+0] = (drflac_int32)((drflac_uint32)((drflac_int32)(mid + side) >> 1) << unusedBitsPerSample);
        pOutputSamples[i*2+1] = (drflac_int32)((drflac_uint32)((drflac_int32)(mid - side) >> 1) << unusedBitsPerSample);
    }
}
#endif

static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_mid_side__scalar_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples_l, drflac_int32* pOutputSamples_r)
{
    drflac_uint64 i;
    drflac_uint64 frameCount4 = frameCount >> 2;
    const drflac_uint32* pInputSamples0U32 = (const drflac_uint32*)pInputSamples0;
    const drflac_uint32* pInputSamples1U32 = (const drflac_uint32*)pInputSamples1;
    drflac_int32 shift = unusedBitsPerSample;

    if (shift > 0) {
        shift -= 1;
        for (i = 0; i < frameCount4; ++i) {
            drflac_uint32 temp0L;
            drflac_uint32 temp1L;
            drflac_uint32 temp2L;
            drflac_uint32 temp3L;
            drflac_uint32 temp0R;
            drflac_uint32 temp1R;
            drflac_uint32 temp2R;
            drflac_uint32 temp3R;

            drflac_uint32 mid0  = pInputSamples0U32[i*4+0] << pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
            drflac_uint32 mid1  = pInputSamples0U32[i*4+1] << pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
            drflac_uint32 mid2  = pInputSamples0U32[i*4+2] << pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
            drflac_uint32 mid3  = pInputSamples0U32[i*4+3] << pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;

            drflac_uint32 side0 = pInputSamples1U32[i*4+0] << pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;
            drflac_uint32 side1 = pInputSamples1U32[i*4+1] << pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;
            drflac_uint32 side2 = pInputSamples1U32[i*4+2] << pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;
            drflac_uint32 side3 = pInputSamples1U32[i*4+3] << pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;

            mid0 = (mid0 << 1) | (side0 & 0x01);
            mid1 = (mid1 << 1) | (side1 & 0x01);
            mid2 = (mid2 << 1) | (side2 & 0x01);
            mid3 = (mid3 << 1) | (side3 & 0x01);

            temp0L = (mid0 + side0) << shift;
            temp1L = (mid1 + side1) << shift;
            temp2L = (mid2 + side2) << shift;
            temp3L = (mid3 + side3) << shift;

            temp0R = (mid0 - side0) << shift;
            temp1R = (mid1 - side1) << shift;
            temp2R = (mid2 - side2) << shift;
            temp3R = (mid3 - side3) << shift;

            pOutputSamples_l[i*4+0] = (drflac_int32)temp0L;
            pOutputSamples_r[i*4+0] = (drflac_int32)temp0R;
            pOutputSamples_l[i*4+1] = (drflac_int32)temp1L;
            pOutputSamples_r[i*4+1] = (drflac_int32)temp1R;
            pOutputSamples_l[i*4+2] = (drflac_int32)temp2L;
            pOutputSamples_r[i*4+2] = (drflac_int32)temp2R;
            pOutputSamples_l[i*4+3] = (drflac_int32)temp3L;
            pOutputSamples_r[i*4+3] = (drflac_int32)temp3R;
        }
    } else {
        for (i = 0; i < frameCount4; ++i) {
            drflac_uint32 temp0L;
            drflac_uint32 temp1L;
            drflac_uint32 temp2L;
            drflac_uint32 temp3L;
            drflac_uint32 temp0R;
            drflac_uint32 temp1R;
            drflac_uint32 temp2R;
            drflac_uint32 temp3R;

            drflac_uint32 mid0  = pInputSamples0U32[i*4+0] << pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
            drflac_uint32 mid1  = pInputSamples0U32[i*4+1] << pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
            drflac_uint32 mid2  = pInputSamples0U32[i*4+2] << pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
            drflac_uint32 mid3  = pInputSamples0U32[i*4+3] << pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;

            drflac_uint32 side0 = pInputSamples1U32[i*4+0] << pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;
            drflac_uint32 side1 = pInputSamples1U32[i*4+1] << pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;
            drflac_uint32 side2 = pInputSamples1U32[i*4+2] << pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;
            drflac_uint32 side3 = pInputSamples1U32[i*4+3] << pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;

            mid0 = (mid0 << 1) | (side0 & 0x01);
            mid1 = (mid1 << 1) | (side1 & 0x01);
            mid2 = (mid2 << 1) | (side2 & 0x01);
            mid3 = (mid3 << 1) | (side3 & 0x01);

            temp0L = (drflac_uint32)((drflac_int32)(mid0 + side0) >> 1);
            temp1L = (drflac_uint32)((drflac_int32)(mid1 + side1) >> 1);
            temp2L = (drflac_uint32)((drflac_int32)(mid2 + side2) >> 1);
            temp3L = (drflac_uint32)((drflac_int32)(mid3 + side3) >> 1);

            temp0R = (drflac_uint32)((drflac_int32)(mid0 - side0) >> 1);
            temp1R = (drflac_uint32)((drflac_int32)(mid1 - side1) >> 1);
            temp2R = (drflac_uint32)((drflac_int32)(mid2 - side2) >> 1);
            temp3R = (drflac_uint32)((drflac_int32)(mid3 - side3) >> 1);

            pOutputSamples_l[i*4+0] = (drflac_int32)temp0L;
            pOutputSamples_r[i*4+0] = (drflac_int32)temp0R;
            pOutputSamples_l[i*4+1] = (drflac_int32)temp1L;
            pOutputSamples_r[i*4+1] = (drflac_int32)temp1R;
            pOutputSamples_l[i*4+2] = (drflac_int32)temp2L;
            pOutputSamples_r[i*4+2] = (drflac_int32)temp2R;
            pOutputSamples_l[i*4+3] = (drflac_int32)temp3L;
            pOutputSamples_r[i*4+3] = (drflac_int32)temp3R;
        }
    }

    for (i = (frameCount4 << 2); i < frameCount; ++i) {
        drflac_uint32 mid  = pInputSamples0U32[i] << pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
        drflac_uint32 side = pInputSamples1U32[i] << pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;

        mid = (mid << 1) | (side & 0x01);

        pOutputSamples_l[i] = (drflac_int32)((drflac_uint32)((drflac_int32)(mid + side) >> 1) << unusedBitsPerSample);
        pOutputSamples_r[i] = (drflac_int32)((drflac_uint32)((drflac_int32)(mid - side) >> 1) << unusedBitsPerSample);
    }
}

static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_mid_side_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples_l, drflac_int32* pOutputSamples_r)
{
    {
        /* Scalar fallback. */
#if 0
        drflac_read_pcm_frames_s32__decode_mid_side__reference_deinterleaved(pFlac, frameCount, unusedBitsPerSample, pInputSamples0, pInputSamples1, pOutputSamples);
#else
        drflac_read_pcm_frames_s32__decode_mid_side__scalar_deinterleaved(pFlac, frameCount, unusedBitsPerSample, pInputSamples0, pInputSamples1, pOutputSamples_l, pOutputSamples_r);
#endif
    }
}


#if 0
static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_independent_stereo__reference_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples)
{
    for (drflac_uint64 i = 0; i < frameCount; ++i) {
        pOutputSamples[i*2+0] = (drflac_int32)((drflac_uint32)pInputSamples0[i] << (unusedBitsPerSample + pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample));
        pOutputSamples[i*2+1] = (drflac_int32)((drflac_uint32)pInputSamples1[i] << (unusedBitsPerSample + pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample));
    }
}
#endif

static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_independent_stereo__scalar_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples_l, drflac_int32* pOutputSamples_r)
{
    drflac_uint64 i;
    drflac_uint64 frameCount4 = frameCount >> 2;
    const drflac_uint32* pInputSamples0U32 = (const drflac_uint32*)pInputSamples0;
    const drflac_uint32* pInputSamples1U32 = (const drflac_uint32*)pInputSamples1;
    drflac_uint32 shift0 = unusedBitsPerSample + pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample;
    drflac_uint32 shift1 = unusedBitsPerSample + pFlac->currentFLACFrame.subframes[1].wastedBitsPerSample;

    for (i = 0; i < frameCount4; ++i) {
        drflac_uint32 tempL0 = pInputSamples0U32[i*4+0] << shift0;
        drflac_uint32 tempL1 = pInputSamples0U32[i*4+1] << shift0;
        drflac_uint32 tempL2 = pInputSamples0U32[i*4+2] << shift0;
        drflac_uint32 tempL3 = pInputSamples0U32[i*4+3] << shift0;

        drflac_uint32 tempR0 = pInputSamples1U32[i*4+0] << shift1;
        drflac_uint32 tempR1 = pInputSamples1U32[i*4+1] << shift1;
        drflac_uint32 tempR2 = pInputSamples1U32[i*4+2] << shift1;
        drflac_uint32 tempR3 = pInputSamples1U32[i*4+3] << shift1;

        pOutputSamples_l[i*4+0] = (drflac_int32)tempL0;
        pOutputSamples_r[i*4+0] = (drflac_int32)tempR0;
        pOutputSamples_l[i*4+1] = (drflac_int32)tempL1;
        pOutputSamples_r[i*4+1] = (drflac_int32)tempR1;
        pOutputSamples_l[i*4+2] = (drflac_int32)tempL2;
        pOutputSamples_r[i*4+2] = (drflac_int32)tempR2;
        pOutputSamples_l[i*4+3] = (drflac_int32)tempL3;
        pOutputSamples_r[i*4+3] = (drflac_int32)tempR3;
    }

    for (i = (frameCount4 << 2); i < frameCount; ++i) {
        pOutputSamples_l[i] = (drflac_int32)(pInputSamples0U32[i] << shift0);
        pOutputSamples_r[i] = (drflac_int32)(pInputSamples1U32[i] << shift1);
    }
}

static DRFLAC_INLINE void drflac_read_pcm_frames_s32__decode_independent_stereo_deinterleaved(drflac* pFlac, drflac_uint64 frameCount, drflac_uint32 unusedBitsPerSample, const drflac_int32* pInputSamples0, const drflac_int32* pInputSamples1, drflac_int32* pOutputSamples_l, drflac_int32* pOutputSamples_r)
{
    {
        /* Scalar fallback. */
#if 0
        drflac_read_pcm_frames_s32__decode_independent_stereo__reference_deinterleaved(pFlac, frameCount, unusedBitsPerSample, pInputSamples0, pInputSamples1, pOutputSamples);
#else
        drflac_read_pcm_frames_s32__decode_independent_stereo__scalar_deinterleaved(pFlac, frameCount, unusedBitsPerSample, pInputSamples0, pInputSamples1, pOutputSamples_l, pOutputSamples_r);
#endif
    }
}


DRFLAC_API drflac_uint64 drflac_read_pcm_frames_s32_deinterleaved(drflac* pFlac, drflac_uint64 framesToRead, drflac_int32* pBufferOut_l, drflac_int32* pBufferOut_r)
{
    drflac_uint64 framesRead;
    drflac_uint32 unusedBitsPerSample;

    if (pFlac == NULL || framesToRead == 0) {
        return 0;
    }

    if (pBufferOut_l == NULL) {
        return drflac__seek_forward_by_pcm_frames(pFlac, framesToRead);
    }

    DRFLAC_ASSERT(pFlac->bitsPerSample <= 32);
    unusedBitsPerSample = 32 - pFlac->bitsPerSample;

    framesRead = 0;
    while (framesToRead > 0) {
        /* If we've run out of samples in this frame, go to the next. */
        if (pFlac->currentFLACFrame.pcmFramesRemaining == 0) {
            if (!drflac__read_and_decode_next_flac_frame(pFlac)) {
                break;  /* Couldn't read the next frame, so just break from the loop and return. */
            }
        } else {
            unsigned int channelCount = drflac__get_channel_count_from_channel_assignment(pFlac->currentFLACFrame.header.channelAssignment);
            drflac_uint64 iFirstPCMFrame = pFlac->currentFLACFrame.header.blockSizeInPCMFrames - pFlac->currentFLACFrame.pcmFramesRemaining;
            drflac_uint64 frameCountThisIteration = framesToRead;

            if (frameCountThisIteration > pFlac->currentFLACFrame.pcmFramesRemaining) {
                frameCountThisIteration = pFlac->currentFLACFrame.pcmFramesRemaining;
            }

            if (channelCount == 2) {
                const drflac_int32* pDecodedSamples0 = pFlac->currentFLACFrame.subframes[0].pSamplesS32 + iFirstPCMFrame;
                const drflac_int32* pDecodedSamples1 = pFlac->currentFLACFrame.subframes[1].pSamplesS32 + iFirstPCMFrame;

                switch (pFlac->currentFLACFrame.header.channelAssignment)
                {
                    case DRFLAC_CHANNEL_ASSIGNMENT_LEFT_SIDE:
                    {
                        drflac_read_pcm_frames_s32__decode_left_side_deinterleaved(pFlac, frameCountThisIteration, unusedBitsPerSample, pDecodedSamples0, pDecodedSamples1, pBufferOut_l, pBufferOut_r);
                    } break;

                    case DRFLAC_CHANNEL_ASSIGNMENT_RIGHT_SIDE:
                    {
                        drflac_read_pcm_frames_s32__decode_right_side_deinterleaved(pFlac, frameCountThisIteration, unusedBitsPerSample, pDecodedSamples0, pDecodedSamples1, pBufferOut_l, pBufferOut_r);
                    } break;

                    case DRFLAC_CHANNEL_ASSIGNMENT_MID_SIDE:
                    {
                        drflac_read_pcm_frames_s32__decode_mid_side_deinterleaved(pFlac, frameCountThisIteration, unusedBitsPerSample, pDecodedSamples0, pDecodedSamples1, pBufferOut_l, pBufferOut_r);
                    } break;

                    case DRFLAC_CHANNEL_ASSIGNMENT_INDEPENDENT:
                    default:
                    {
                        drflac_read_pcm_frames_s32__decode_independent_stereo_deinterleaved(pFlac, frameCountThisIteration, unusedBitsPerSample, pDecodedSamples0, pDecodedSamples1, pBufferOut_l, pBufferOut_r);
                    } break;
                }
                pBufferOut_r              += frameCountThisIteration;
            } else if (channelCount == 1){
                for (drflac_uint64 i = 0; i < frameCountThisIteration; ++i) {
                    pBufferOut_l[i] = (drflac_int32)((drflac_uint32)(pFlac->currentFLACFrame.subframes[0].pSamplesS32[iFirstPCMFrame + i]) << (unusedBitsPerSample + pFlac->currentFLACFrame.subframes[0].wastedBitsPerSample));
                }
            } else {
                assert(0 && "Error: not mono or stereo!");
            }

            framesRead                += frameCountThisIteration;
            pBufferOut_l              += frameCountThisIteration;
            framesToRead              -= frameCountThisIteration;
            pFlac->currentPCMFrame    += frameCountThisIteration;
            pFlac->currentFLACFrame.pcmFramesRemaining -= (drflac_uint32)frameCountThisIteration;
        }
    }
    return framesRead;
}

DRFLAC_API drflac* drflac_open_memory_relaxed(const void* pData, size_t dataSize, drflac_container container, const drflac_allocation_callbacks* pAllocationCallbacks)
{
    drflac__memory_stream memoryStream;
    drflac* pFlac;

    memoryStream.data = (const drflac_uint8*)pData;
    memoryStream.dataSize = dataSize;
    memoryStream.currentReadPos = 0;
    pFlac = drflac_open_relaxed(drflac__on_read_memory, drflac__on_seek_memory, container, &memoryStream, pAllocationCallbacks);
    if (pFlac == NULL) {
        return NULL;
    }

    pFlac->memoryStream = memoryStream;

    /* This is an awful hack... */
#ifndef DR_FLAC_NO_OGG
    if (pFlac->container == drflac_container_ogg)
    {
        drflac_oggbs* oggbs = (drflac_oggbs*)pFlac->_oggbs;
        oggbs->pUserData = &pFlac->memoryStream;
    }
    else
#endif
    {
        pFlac->bs.pUserData = &pFlac->memoryStream;
    }

    return pFlac;
}

static void *hmalloc(size_t s, void *d) {
    (void) d;
    return malloc(s);
}

static void hfree(void *p, void *d) {
    (void) d;
    free(p);
}
