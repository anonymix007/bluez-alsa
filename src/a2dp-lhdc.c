/*
 * BlueALSA - a2dp-lhdc.c
 * Copyright (c) 2016-2023 Arkadiusz Bokowy
 * Copyright (c) 2023      anonymix007
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include "a2dp-lhdc.h"

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <glib.h>

#include <lhdcBT.h>
#include <lhdcBT_dec.h>

#include "a2dp.h"
#include "audio.h"
#include "ba-transport-pcm.h"
#include "bluealsa-config.h"
#include "io.h"
#include "rtp.h"
#include "utils.h"
#include "shared/a2dp-codecs.h"
#include "shared/defs.h"
#include "shared/ffb.h"
#include "shared/log.h"
#include "shared/rt.h"

static const struct a2dp_channel_mode a2dp_lhdc_channels[] = {
	{ A2DP_CHM_STEREO, 2, LHDC_CHANNEL_MODE_STEREO },
};

static const struct a2dp_sampling_freq a2dp_lhdc_samplings[] = {
	{ 44100, LHDC_SAMPLING_FREQ_44100 },
	{ 48000, LHDC_SAMPLING_FREQ_48000 },
	{ 96000, LHDC_SAMPLING_FREQ_96000 },
};

struct a2dp_codec a2dp_lhdc_sink = {
	.dir = A2DP_SINK,
	.codec_id = A2DP_CODEC_VENDOR_LHDC_V3,
	.capabilities.lhdc_v3 = {
		.info = A2DP_SET_VENDOR_ID_CODEC_ID(LHDC_V3_VENDOR_ID, LHDC_V3_CODEC_ID),
		.frequency =
			LHDC_SAMPLING_FREQ_44100 |
			LHDC_SAMPLING_FREQ_48000 |
			LHDC_SAMPLING_FREQ_96000,
		.bit_depth =
			LHDC_BIT_DEPTH_16 |
			LHDC_BIT_DEPTH_24,
		.jas = 0,
		.ar = 0,
		.version = LHDC_VER3,
		.max_bit_rate = LHDC_MAX_BIT_RATE_900K,
		.low_latency = 0,
		.llac = 1,
		.ch_split_mode = LHDC_CH_SPLIT_MODE_NONE,
		.meta = 0,
		.min_bitrate = 0,
		.larc = 0,
		.lhdc_v4 = 1,
	},
	.capabilities_size = sizeof(a2dp_lhdc_v3_t),
	.channels[0] = a2dp_lhdc_channels,
	.channels_size[0] = ARRAYSIZE(a2dp_lhdc_channels),
	.samplings[0] = a2dp_lhdc_samplings,
	.samplings_size[0] = ARRAYSIZE(a2dp_lhdc_samplings),
};

struct a2dp_codec a2dp_lhdc_source = {
	.dir = A2DP_SOURCE,
	.codec_id = A2DP_CODEC_VENDOR_LHDC_V3,
	.capabilities.lhdc_v3 = {
		.info = A2DP_SET_VENDOR_ID_CODEC_ID(LHDC_V3_VENDOR_ID, LHDC_V3_CODEC_ID),
		.frequency =
			LHDC_SAMPLING_FREQ_44100 |
			LHDC_SAMPLING_FREQ_48000 |
			LHDC_SAMPLING_FREQ_96000,
		.bit_depth =
			LHDC_BIT_DEPTH_16 |
			LHDC_BIT_DEPTH_24,
		.jas = 0,
		.ar = 0,
		.version = LHDC_VER3,
		.max_bit_rate = LHDC_MAX_BIT_RATE_900K,
		.low_latency = 0,
		.llac = 0, // TODO: copy LLAC/V3/V4 logic from AOSP patches
		.ch_split_mode = LHDC_CH_SPLIT_MODE_NONE,
		.meta = 0,
		.min_bitrate = 0,
		.larc = 0,
		.lhdc_v4 = 1,
	},
	.capabilities_size = sizeof(a2dp_lhdc_v3_t),
	.channels[0] = a2dp_lhdc_channels,
	.channels_size[0] = ARRAYSIZE(a2dp_lhdc_channels),
	.samplings[0] = a2dp_lhdc_samplings,
	.samplings_size[0] = ARRAYSIZE(a2dp_lhdc_samplings),
};

void a2dp_lhdc_init(void) {
}

void a2dp_lhdc_transport_init(struct ba_transport *t) {

	const struct a2dp_codec *codec = t->a2dp.codec;

	if (codec->dir == A2DP_SINK) {
		t->a2dp.pcm.format = BA_TRANSPORT_PCM_FORMAT_S24_4LE;
	} else {
		t->a2dp.pcm.format = BA_TRANSPORT_PCM_FORMAT_S32_4LE;
	}	

	t->a2dp.pcm.channels = a2dp_codec_lookup_channels(codec,
			LHDC_CHANNEL_MODE_STEREO, false);
	t->a2dp.pcm.sampling = a2dp_codec_lookup_frequency(codec,
			t->a2dp.configuration.lhdc_v3.frequency, false);

}

static LHDC_VERSION_SETUP get_version(const a2dp_lhdc_v3_t *configuration) {
	if (configuration->llac) {
		return LLAC;
	} else if (configuration->lhdc_v4) {
		return LHDC_V4;
	} else {
		return LHDC_V3;
	}
}

static int get_encoder_interval(const a2dp_lhdc_v3_t *configuration) {
	if (configuration->low_latency) {
		return 10;
	} else {
		return 20;
	}
}

static int get_bit_depth(const a2dp_lhdc_v3_t *configuration) {
	if (configuration->bit_depth == LHDC_BIT_DEPTH_16) {
		return 16;
	} else {
		return 24;
	}
}

static LHDCBT_QUALITY_T get_max_bitrate(const a2dp_lhdc_v3_t *configuration) {
	if (configuration->max_bit_rate == LHDC_MAX_BIT_RATE_400K) {
		return LHDCBT_QUALITY_LOW;
	} else if (configuration->max_bit_rate == LHDC_MAX_BIT_RATE_500K) {
		return LHDCBT_QUALITY_MID;
	} else {
		return LHDCBT_QUALITY_HIGH;
	}
}

void *a2dp_lhdc_enc_thread(struct ba_transport_pcm *t_pcm) {

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_push(PTHREAD_CLEANUP(ba_transport_pcm_thread_cleanup), t_pcm);

	struct ba_transport *t = t_pcm->t;
	struct ba_transport_thread *th = t_pcm->th;
	struct io_poll io = { .timeout = -1 };

	const a2dp_lhdc_v3_t *configuration = &t->a2dp.configuration.lhdc_v3;

	HANDLE_LHDC_BT handle;
	if ((handle = lhdcBT_get_handle(get_version(configuration))) == NULL) {
		error("Couldn't get LHDC handle: %s", strerror(errno));
		goto fail_open_lhdc;
	}

	pthread_cleanup_push(PTHREAD_CLEANUP(lhdcBT_free_handle), handle);

	const size_t sample_size = BA_TRANSPORT_PCM_FORMAT_BYTES(t_pcm->format);
	const unsigned int channels = t_pcm->channels;
	const unsigned int samplerate = t_pcm->sampling;
	const unsigned int bitdepth = get_bit_depth(configuration);

	lhdcBT_set_hasMinBitrateLimit(handle, configuration->min_bitrate);
	lhdcBT_set_max_bitrate(handle, get_max_bitrate(configuration));

	if (lhdcBT_init_encoder(handle, samplerate, bitdepth, config.lhdc_eqmid,
			configuration->ch_split_mode > LHDC_CH_SPLIT_MODE_NONE, 0, t->mtu_write,
			get_encoder_interval(configuration)) == -1) {
		error("Couldn't initialize LHDC encoder");
		goto fail_init;
	}

	const size_t lhdc_ch_samples = lhdcBT_get_block_Size(handle);
	const size_t lhdc_pcm_samples = lhdc_ch_samples * channels;

	ffb_t bt = { 0 };
	ffb_t pcm = { 0 };
	pthread_cleanup_push(PTHREAD_CLEANUP(ffb_free), &bt);
	pthread_cleanup_push(PTHREAD_CLEANUP(ffb_free), &pcm);

	int32_t *pcm_ch1 = malloc(lhdc_ch_samples * sizeof(int32_t));
	int32_t *pcm_ch2 = malloc(lhdc_ch_samples * sizeof(int32_t));
	pthread_cleanup_push(PTHREAD_CLEANUP(free), pcm_ch1);
	pthread_cleanup_push(PTHREAD_CLEANUP(free), pcm_ch2);

	if (ffb_init_int32_t(&pcm, lhdc_pcm_samples) == -1 ||
			ffb_init_uint8_t(&bt, t->mtu_write) == -1) {
		error("Couldn't create data buffers: %s", strerror(errno));
		goto fail_ffb;
	}

	rtp_header_t *rtp_header;
	struct {
		uint8_t seq_num;
		uint8_t latency:2;
		uint8_t frames:6;
	} *lhdc_media_header;
	/* initialize RTP headers and get anchor for payload */
	uint8_t *rtp_payload = rtp_a2dp_init(bt.data, &rtp_header,
			(void **)&lhdc_media_header, sizeof(*lhdc_media_header));

	struct rtp_state rtp = { .synced = false };
	/* RTP clock frequency equal to audio samplerate */
	rtp_state_init(&rtp, samplerate, samplerate);

	debug_transport_pcm_thread_loop(t_pcm, "START");

	uint8_t seq_num = 0;

	for (ba_transport_thread_state_set_running(th);;) {

		ssize_t samples = ffb_len_in(&pcm);
		switch (samples = io_poll_and_read_pcm(&io, t_pcm, pcm.tail, samples)) {
		case -1:
			if (errno == ESTALE) {
				ffb_rewind(&pcm);
				continue;
			}
			error("PCM poll and read error: %s", strerror(errno));
			/* fall-through */
		case 0:
			ba_transport_stop_if_no_clients(t);
			continue;
		}

		ffb_seek(&pcm, samples);
		samples = ffb_len_out(&pcm);

		int *input = pcm.data;
		size_t input_len = samples;

		/* encode and transfer obtained data */
		while (input_len >= lhdc_pcm_samples) {

			/* anchor for RTP payload */
			bt.tail = rtp_payload;

			audio_deinterleave_s32_4le(input, lhdc_ch_samples, channels, pcm_ch1, pcm_ch2);

			uint32_t encoded;
			uint32_t frames;

			if (lhdcBT_encode_stereo(handle, pcm_ch1, pcm_ch2, bt.tail, &encoded, &frames) < 0) {
				error("LHDC encoding error");
				break;
			}

			input += lhdc_pcm_samples;
			input_len -= lhdc_pcm_samples;
			ffb_seek(&bt, encoded);

			if (encoded > 0) {

				lhdc_media_header->seq_num = seq_num++;
				lhdc_media_header->latency = 0;
				lhdc_media_header->frames = frames;

				rtp_state_new_frame(&rtp, rtp_header);

				/* Try to get the number of bytes queued in the
				 * socket output buffer. */
				int queued_bytes = 0;
				if (ioctl(t->bt_fd, TIOCOUTQ, &queued_bytes) != -1)
					queued_bytes = abs(t->a2dp.bt_fd_coutq_init - queued_bytes);

				errno = 0;

				ssize_t len = ffb_blen_out(&bt);
				if ((len = io_bt_write(th, bt.data, len)) <= 0) {
					if (len == -1)
						error("BT write error: %s", strerror(errno));
					goto fail;
				}

				if (errno == EAGAIN)
					/* The io_bt_write() call was blocking due to not enough
					 * space in the BT socket. Set the queued_bytes to some
					 * arbitrary big value. */
					queued_bytes = 1024 * 16;

				if (config.lhdc_eqmid == LHDCBT_QUALITY_AUTO)
					lhdcBT_adjust_bitrate(handle, queued_bytes / t->mtu_write);
			}

			unsigned int pcm_frames = lhdc_pcm_samples / channels;
			/* keep data transfer at a constant bit rate */
			asrsync_sync(&io.asrs, pcm_frames);
			/* move forward RTP timestamp clock */
			rtp_state_update(&rtp, pcm_frames);

			/* update busy delay (encoding overhead) */
			t_pcm->delay = asrsync_get_busy_usec(&io.asrs) / 100;

		}

		/* If the input buffer was not consumed (due to codesize limit), we
		 * have to append new data to the existing one. Since we do not use
		 * ring buffer, we will simply move unprocessed data to the front
		 * of our linear buffer. */
		ffb_shift(&pcm, samples - input_len);

	}

fail:
	debug_transport_pcm_thread_loop(t_pcm, "EXIT");
fail_ffb:
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
fail_init:
	pthread_cleanup_pop(1);
fail_open_lhdc:
	pthread_cleanup_pop(1);
	return NULL;
}

static const int versions[5] = {
	[LHDC_V2] = VERSION_2,
	[LHDC_V3] = VERSION_3,
	[LHDC_V4] = VERSION_4,
	[LLAC]	= VERSION_LLAC,
};

void *a2dp_lhdc_dec_thread(struct ba_transport_pcm *t_pcm) {

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_push(PTHREAD_CLEANUP(ba_transport_pcm_thread_cleanup), t_pcm);

	struct ba_transport *t = t_pcm->t;
	struct ba_transport_thread *th = t_pcm->th;
	struct io_poll io = { .timeout = -1 };

	const a2dp_lhdc_v3_t *configuration = &t->a2dp.configuration.lhdc_v3;
	const size_t sample_size = BA_TRANSPORT_PCM_FORMAT_BYTES(t_pcm->format);
	const unsigned int channels = t_pcm->channels;
	const unsigned int samplerate = t_pcm->sampling;
	const unsigned int bitdepth = get_bit_depth(configuration);

	tLHDCV3_DEC_CONFIG dec_config = {
		.version = versions[get_version(configuration)],
		.sample_rate = samplerate,
		.bits_depth = bitdepth,
	};

	if (lhdcBT_dec_init_decoder(&dec_config) < 0) {
		error("Couldn't initialise LHDC decoder: %s", strerror(errno));
		goto fail_open;
	}

	pthread_cleanup_push(PTHREAD_CLEANUP(lhdcBT_dec_deinit_decoder), NULL);

	ffb_t bt = { 0 };
	ffb_t pcm = { 0 };
	pthread_cleanup_push(PTHREAD_CLEANUP(ffb_free), &bt);
	pthread_cleanup_push(PTHREAD_CLEANUP(ffb_free), &pcm);

	if (ffb_init_int32_t(&pcm, 16 * 256 * channels) == -1 ||
			ffb_init_uint8_t(&bt, t->mtu_read) == -1) {
		error("Couldn't create data buffers: %s", strerror(errno));
		goto fail_ffb;
	}

	struct rtp_state rtp = { .synced = false };
	/* RTP clock frequency equal to audio samplerate */
	rtp_state_init(&rtp, samplerate, samplerate);

	debug_transport_pcm_thread_loop(t_pcm, "START");
	for (ba_transport_thread_state_set_running(th);;) {

		ssize_t len = ffb_blen_in(&bt);
		if ((len = io_poll_and_read_bt(&io, th, bt.data, len)) <= 0) {
			if (len == -1)
				error("BT poll and read error: %s", strerror(errno));
			goto fail;
		}

		const rtp_header_t *rtp_header = bt.data;
		const void *lhdc_media_header;
		if ((lhdc_media_header = rtp_a2dp_get_payload(rtp_header)) == NULL)
			continue;

		int missing_rtp_frames = 0;
		rtp_state_sync_stream(&rtp, rtp_header, &missing_rtp_frames, NULL);

		if (!ba_transport_pcm_is_active(t_pcm)) {
			rtp.synced = false;
			continue;
		}

		const uint8_t *rtp_payload = (uint8_t *) lhdc_media_header;
		size_t rtp_payload_len = len - (rtp_payload - (uint8_t *)bt.data);

		uint32_t decoded = 16 * 256 * sizeof(int32_t) * channels;

		lhdcBT_dec_decode(rtp_payload, rtp_payload_len, pcm.data, &decoded, 24);

		const size_t samples = decoded / sample_size;
		io_pcm_scale(t_pcm, pcm.data, samples);
		if (io_pcm_write(t_pcm, pcm.data, samples) == -1)
			error("FIFO write error: %s", strerror(errno));

		/* update local state with decoded PCM frames */
		rtp_state_update(&rtp, samples / channels);

	}

fail:
	debug_transport_pcm_thread_loop(t_pcm, "EXIT");
fail_ffb:
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
fail_open:
	pthread_cleanup_pop(1);
	return NULL;
}

int a2dp_lhdc_transport_start(struct ba_transport *t) {

	if (t->profile & BA_TRANSPORT_PROFILE_A2DP_SOURCE)
		return ba_transport_pcm_start(&t->a2dp.pcm, a2dp_lhdc_enc_thread, "ba-a2dp-lhdc", true);

	if (t->profile & BA_TRANSPORT_PROFILE_A2DP_SINK)
		return ba_transport_pcm_start(&t->a2dp.pcm, a2dp_lhdc_dec_thread, "ba-a2dp-lhdc", true);

	g_assert_not_reached();
	return -1;
}
