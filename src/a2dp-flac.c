/*
 * BlueALSA - a2dp-flac.c
 * Copyright (c) 2023 Arkadiusz Bokowy
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include "a2dp-flac.h"
/* IWYU pragma: no_include "config.h" */

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>

#include "FLAC.h"

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

static const struct a2dp_channel_mode a2dp_flac_channels[] = {
	{ A2DP_CHM_MONO, 1, FLAC_CHANNELS_1 },
	{ A2DP_CHM_STEREO, 2, FLAC_CHANNELS_2 },
};

static const struct a2dp_sampling_freq a2dp_flac_samplings[] = {
	{ 44100, FLAC_SAMPLING_FREQ_44100 },
	{ 48000, FLAC_SAMPLING_FREQ_48000 },
	{ 96000, FLAC_SAMPLING_FREQ_96000 },
};

struct a2dp_codec a2dp_flac_sink = {
	.dir = A2DP_SINK,
	.codec_id = A2DP_CODEC_VENDOR_FLAC,
	.capabilities.flac = {
		.info = A2DP_SET_VENDOR_ID_CODEC_ID(FLAC_VENDOR_ID, FLAC_CODEC_ID),
		.bits_per_sample =
			FLAC_BITS_PER_SAMPLE_16,
		.channels =
			FLAC_CHANNELS_1 |
			FLAC_CHANNELS_2,
		FLAC_INIT_FREQUENCY(FLAC_SAMPLING_FREQ_44100)
	},
	.capabilities_size = sizeof(a2dp_flac_t),
	.channels[0] = a2dp_flac_channels,
	.channels_size[0] = ARRAYSIZE(a2dp_flac_channels),
	.samplings[0] = a2dp_flac_samplings,
	.samplings_size[0] = ARRAYSIZE(a2dp_flac_samplings),
};

struct a2dp_codec a2dp_flac_source = {
	.dir = A2DP_SOURCE,
	.codec_id = A2DP_CODEC_VENDOR_FLAC,
	.capabilities.flac = {
		.info = A2DP_SET_VENDOR_ID_CODEC_ID(FLAC_VENDOR_ID, FLAC_CODEC_ID),
		.bits_per_sample =
			FLAC_BITS_PER_SAMPLE_16,
		.channels =
			FLAC_CHANNELS_1 |
			FLAC_CHANNELS_2,
		FLAC_INIT_FREQUENCY(FLAC_SAMPLING_FREQ_44100 | FLAC_SAMPLING_FREQ_48000 | FLAC_SAMPLING_FREQ_96000 )
	},
	.capabilities_size = sizeof(a2dp_flac_t),
	.channels[0] = a2dp_flac_channels,
	.channels_size[0] = ARRAYSIZE(a2dp_flac_channels),
	.samplings[0] = a2dp_flac_samplings,
	.samplings_size[0] = ARRAYSIZE(a2dp_flac_samplings),
};

void a2dp_flac_init(void) {
}

void a2dp_flac_transport_init(struct ba_transport *t) {

	const struct a2dp_codec *codec = t->a2dp.codec;

	t->a2dp.pcm.format = BA_TRANSPORT_PCM_FORMAT_S32_4LE;
	t->a2dp.pcm.channels = a2dp_codec_lookup_channels(codec,
			t->a2dp.configuration.flac.channels, false);
	t->a2dp.pcm.sampling = a2dp_codec_lookup_frequency(codec,
			FLAC_GET_FREQUENCY(t->a2dp.configuration.flac), false);

}

static bool flac_channels_supported(int channels) {
    return channels == 1 || channels == 2;
}
static bool flac_samplerate_supported(int samplerate) {
    return samplerate == 44100 || samplerate == 48000 | samplerate == 96000;
}

static bool a2dp_flac_supported(int samplerate, int channels) {

	if (flac_channels_supported(channels) == 0) {
		error("Number of channels not supported by FLAC library: %u", channels);
		return false;
	}

	if (flac_samplerate_supported(samplerate) == 0) {
		error("Sampling frequency not supported by FLAC library: %u", samplerate);
		return false;
	}

	return true;
}

static void *a2dp_flac_enc_init(int samplerate, int channels) {
	return NULL;
}

static void a2dp_flac_enc_free(void *handle) {
	if (handle == NULL)
		return;
	free(handle);
}

void *a2dp_flac_enc_thread(struct ba_transport_pcm *t_pcm) {
#if 0
	/* Cancellation should be possible only in the carefully selected place
	 * in order to prevent memory leaks and resources not being released. */
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_push(PTHREAD_CLEANUP(ba_transport_pcm_thread_cleanup), t_pcm);

	struct ba_transport *t = t_pcm->t;
	struct ba_transport_thread *th = t_pcm->th;
	struct io_poll io = { .timeout = -1 };

	const a2dp_flac_t *configuration = &t->a2dp.configuration.flac;
	const int flac_frame_dms = a2dp_flac_get_frame_dms(configuration);
	const unsigned int channels = t_pcm->channels;
	const unsigned int samplerate = t_pcm->sampling;
	const unsigned int rtp_ts_clockrate = 96000;

	/* check whether library supports selected configuration */
	if (!a2dp_flac_supported(samplerate, channels))
		goto fail_init;

	FLAC_Enc *handle;
	FLAC_Error err;

	if ((handle = a2dp_flac_enc_init(samplerate, channels)) == NULL) {
		error("Couldn't initialize FLAC codec: %s", strerror(errno));
		goto fail_init;
	}

	pthread_cleanup_push(PTHREAD_CLEANUP(a2dp_flac_enc_free), handle);

	if ((err = flac_enc_set_frame_dms(handle, flac_frame_dms)) != FLAC_OK) {
		error("Couldn't set frame length: %s", flac_strerror(err));
		goto fail_setup;
	}
	if ((err = flac_enc_set_bitrate(handle, config.flac_bitrate)) != FLAC_OK) {
		error("Couldn't set bitrate: %s", flac_strerror(err));
		goto fail_setup;
	}

	ffb_t bt = { 0 };
	ffb_t pcm = { 0 };
	pthread_cleanup_push(PTHREAD_CLEANUP(ffb_free), &bt);
	pthread_cleanup_push(PTHREAD_CLEANUP(ffb_free), &pcm);

	const size_t flac_ch_samples = flac_enc_get_input_samples(handle);
	const size_t flac_frame_samples = flac_ch_samples * channels;
	const size_t flac_frame_len = flac_enc_get_num_bytes(handle);

	const size_t rtp_headers_len = RTP_HEADER_LEN + sizeof(rtp_media_header_t);
	const size_t mtu_write_payload_len = t->mtu_write - rtp_headers_len;

	size_t ffb_pcm_len = flac_frame_samples;
	if (mtu_write_payload_len / flac_frame_len > 1)
		/* account for possible FLAC frames packing */
		ffb_pcm_len *= mtu_write_payload_len / flac_frame_len;

	size_t ffb_bt_len = t->mtu_write;
	if (ffb_bt_len < rtp_headers_len + flac_frame_len)
		/* bigger than MTU buffer will be fragmented later */
		ffb_bt_len = rtp_headers_len + flac_frame_len;

	int32_t *pcm_ch1 = malloc(flac_ch_samples * sizeof(int32_t));
	int32_t *pcm_ch2 = malloc(flac_ch_samples * sizeof(int32_t));
	int32_t *pcm_ch_buffers[2] = { pcm_ch1, pcm_ch2 };
	pthread_cleanup_push(PTHREAD_CLEANUP(free), pcm_ch1);
	pthread_cleanup_push(PTHREAD_CLEANUP(free), pcm_ch2);

	if (ffb_init_int32_t(&pcm, ffb_pcm_len) == -1 ||
			ffb_init_uint8_t(&bt, ffb_bt_len) == -1 ||
			pcm_ch1 == NULL || pcm_ch2 == NULL) {
		error("Couldn't create data buffers: %s", strerror(errno));
		goto fail_ffb;
	}

	rtp_header_t *rtp_header;
	rtp_media_header_t *rtp_media_header;
	/* initialize RTP headers and get anchor for payload */
	uint8_t *rtp_payload = rtp_a2dp_init(bt.data, &rtp_header,
			(void **)&rtp_media_header, sizeof(*rtp_media_header));

	struct rtp_state rtp = { .synced = false };
	/* RTP clock frequency equal to the RTP clock rate */
	rtp_state_init(&rtp, samplerate, rtp_ts_clockrate);

	debug_transport_pcm_thread_loop(t_pcm, "START");
	for (ba_transport_thread_state_set_running(th);;) {

		ssize_t samples = ffb_len_in(&pcm);
		switch (samples = io_poll_and_read_pcm(&io, t_pcm, pcm.tail, samples)) {
		case -1:
			if (errno == ESTALE) {
				int encoded = 0;
				void *scratch = NULL;
				memset(pcm_ch1, 0, flac_ch_samples * sizeof(*pcm_ch1));
				memset(pcm_ch2, 0, flac_ch_samples * sizeof(*pcm_ch2));
				/* flush encoder internal buffers by feeding it with silence */
				flac_enc24(handle, pcm_ch_buffers, rtp_payload, &encoded, scratch);
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

		/* anchor for RTP payload */
		bt.tail = rtp_payload;

		const int32_t *input = pcm.data;
		size_t input_samples = samples;
		size_t output_len = ffb_len_in(&bt);
		size_t pcm_frames = 0;
		size_t flac_frames = 0;

		/* pack as many FLAC frames as possible */
		while (input_samples >= flac_frame_samples &&
				output_len >= flac_frame_len &&
				/* RTP packet shall not exceed 20.0 ms of audio */
				flac_frames * flac_frame_dms <= 200 &&
				/* do not overflow RTP frame counter */
				flac_frames < ((1 << 4) - 1)) {

			int encoded = 0;
			void *scratch = NULL;
			audio_deinterleave_s24_4le(input, flac_ch_samples, channels, pcm_ch1, pcm_ch2);
			if ((err = flac_enc24(handle, pcm_ch_buffers, bt.tail, &encoded, scratch)) != FLAC_OK) {
				error("FLAC encoding error: %s", flac_strerror(err));
				break;
			}

			input += flac_frame_samples;
			input_samples -= flac_frame_samples;
			ffb_seek(&bt, encoded);
			output_len -= encoded;
			pcm_frames += flac_ch_samples;
			flac_frames++;

		}

		if (flac_frames > 0) {

			size_t payload_len_max = t->mtu_write - rtp_headers_len;
			size_t payload_len = ffb_blen_out(&bt) - rtp_headers_len;
			memset(rtp_media_header, 0, sizeof(*rtp_media_header));
			rtp_media_header->frame_count = flac_frames;

			/* If the size of the RTP packet exceeds writing MTU, the RTP payload
			 * should be fragmented. The fragmentation scheme is defined by the
			 * vendor specific FLAC Bluetooth A2DP specification. */

			if (payload_len > payload_len_max) {
				rtp_media_header->fragmented = 1;
				rtp_media_header->first_fragment = 1;
				rtp_media_header->frame_count = DIV_ROUND_UP(payload_len, payload_len_max);
			}

			for (;;) {

				size_t chunk_len;
				chunk_len = payload_len > payload_len_max ? payload_len_max : payload_len;
				rtp_state_new_frame(&rtp, rtp_header);

				ffb_rewind(&bt);
				ffb_seek(&bt, rtp_headers_len + chunk_len);

				ssize_t len = ffb_blen_out(&bt);
				if ((len = io_bt_write(th, bt.data, len)) <= 0) {
					if (len == -1)
						error("BT write error: %s", strerror(errno));
					goto fail;
				}

				/* resend RTP headers */
				len -= rtp_headers_len;

				/* break if there is no more payload data */
				if ((payload_len -= len) == 0)
					break;

				/* move the rest of data to the beginning of payload */
				debug("FLAC payload fragmentation: extra %zu bytes", payload_len);
				memmove(rtp_payload, rtp_payload + len, payload_len);

				rtp_media_header->first_fragment = 0;
				rtp_media_header->last_fragment = payload_len <= payload_len_max;
				rtp_media_header->frame_count--;

			}

			/* keep data transfer at a constant bit rate */
			asrsync_sync(&io.asrs, pcm_frames);
			/* move forward RTP timestamp clock */
			rtp_state_update(&rtp, pcm_frames);

			/* update busy delay (encoding overhead) */
			t_pcm->delay = asrsync_get_busy_usec(&io.asrs) / 100;

			/* If the input buffer was not consumed (due to codesize limit), we
			 * have to append new data to the existing one. Since we do not use
			 * ring buffer, we will simply move unprocessed data to the front
			 * of our linear buffer. */
			ffb_shift(&pcm, samples - input_samples);

		}

	}

fail:
	debug_transport_pcm_thread_loop(t_pcm, "EXIT");
fail_ffb:
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
fail_setup:
	pthread_cleanup_pop(1);
fail_init:
	pthread_cleanup_pop(1);
#endif
	return NULL;
}

__attribute__ ((weak))
void *a2dp_flac_dec_thread(struct ba_transport_pcm *t_pcm) {

	/* Cancellation should be possible only in the carefully selected place
	 * in order to prevent memory leaks and resources not being released. */
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_push(PTHREAD_CLEANUP(ba_transport_pcm_thread_cleanup), t_pcm);

	struct ba_transport *t = t_pcm->t;
	struct ba_transport_thread *th = t_pcm->th;
	struct io_poll io = { .timeout = -1 };

	const a2dp_flac_t *configuration = &t->a2dp.configuration.flac;
	const unsigned int channels = t_pcm->channels;
	const unsigned int samplerate = t_pcm->sampling;
	const unsigned int bits_per_sample = configuration->bits_per_sample == FLAC_BITS_PER_SAMPLE_16 ? 16 : 24;

	/* check whether library supports selected configuration */
	if (!a2dp_flac_supported(samplerate, channels))
		goto fail_init;

	ffb_t bt = { 0 };
	ffb_t bt_payload = { 0 };
	ffb_t pcm = { 0 };
	pthread_cleanup_push(PTHREAD_CLEANUP(ffb_free), &bt);
	pthread_cleanup_push(PTHREAD_CLEANUP(ffb_free), &bt_payload);
	pthread_cleanup_push(PTHREAD_CLEANUP(ffb_free), &pcm);



	const size_t flac_ch_samples = (t->mtu_read - 12) * CHAR_BITS / (channels * bits_per_sample);
	const size_t flac_frame_samples = flac_ch_samples * channels;

	debug("FLAC block size: %zu", flac_ch_samples);


	int32_t *pcm_ch1 = malloc(flac_ch_samples * sizeof(int32_t));
	int32_t *pcm_ch2 = malloc(flac_ch_samples * sizeof(int32_t));
	int32_t *pcm_ch_buffers[2] = { pcm_ch1, pcm_ch2 };
	pthread_cleanup_push(PTHREAD_CLEANUP(free), pcm_ch1);
	pthread_cleanup_push(PTHREAD_CLEANUP(free), pcm_ch2);

	if (ffb_init_int32_t(&pcm, flac_frame_samples) == -1 ||
			ffb_init_uint8_t(&bt_payload, t->mtu_read) == -1 ||
			ffb_init_uint8_t(&bt, t->mtu_read) == -1 ||
			pcm_ch1 == NULL || pcm_ch2 == NULL) {
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
		const rtp_media_header_t *rtp_media_header;
		if ((rtp_media_header = rtp_a2dp_get_payload(rtp_header)) == NULL)
			continue;

		int missing_rtp_frames = 0;
		int missing_pcm_frames = 0;
		rtp_state_sync_stream(&rtp, rtp_header, &missing_rtp_frames, &missing_pcm_frames);

		if (!ba_transport_pcm_is_active(t_pcm)) {
			rtp.synced = false;
			continue;
		}

#if DEBUG
		if (missing_pcm_frames > 0) {
			size_t missing_flac_frames = DIV_ROUND_UP(missing_pcm_frames, flac_ch_samples);
			debug("Missing FLAC frames: %zu", missing_flac_frames);
		}
#endif

		uint8_t *flac_payload = (uint8_t *)(rtp_media_header + 1);
		/* For not-fragmented transfer, the frame count shall indicate the number
		 * of FLAC frames within a single RTP payload. In case of fragmented
		 * transfer, the last fragment should have the frame count set to 1. */
		size_t flac_frames = 1;
		size_t flac_frame_len = len - (flac_payload - (uint8_t *)bt.data);

		/* Decode retrieved FLAC frames. */
		while (flac_frames--) {
			drflac_allocation_callbacks alloc = {0};
			alloc.onMalloc = hmalloc;
			alloc.onFree = hfree;

			drflac *dec = drflac_open_memory_relaxed(flac_payload - 12, flac_frame_len + 12, drflac_container_native, &alloc);
			size_t output_samples = drflac_read_pcm_frames_s32_deinterleaved(dec, 512, pcm_ch1, pcm_ch2);
			if (dec == NULL || output_samples == 0) {
				warn("FLAC decoding error: frame length is %zu, decoded samples %zu! Playing silence instead", flac_frame_len, output_samples);
				memset(pcm_ch1, 0, flac_ch_samples * sizeof(int32_t));
				memset(pcm_ch2, 0, flac_ch_samples * sizeof(int32_t));
			}

			audio_interleave_s32_4le(pcm_ch1, pcm_ch2, flac_ch_samples, channels, pcm.data);

			flac_payload += flac_frame_len;

			const size_t samples = flac_frame_samples;
			io_pcm_scale(t_pcm, pcm.data, samples);
			if (io_pcm_write(t_pcm, pcm.data, samples) == -1)
				error("FIFO write error: %s", strerror(errno));

			/* update local state with decoded PCM frames */
			rtp_state_update(&rtp, flac_ch_samples);

		}

		/* make room for new payload */
		ffb_rewind(&bt_payload);

	}

fail:
	debug_transport_pcm_thread_loop(t_pcm, "EXIT");
fail_ffb:
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
fail_init:
	pthread_cleanup_pop(1);
	return NULL;
}

int a2dp_flac_transport_start(struct ba_transport *t) {

	if (t->profile & BA_TRANSPORT_PROFILE_A2DP_SOURCE)
		return ba_transport_pcm_start(&t->a2dp.pcm, a2dp_flac_enc_thread, "ba-a2dp-flac", true);

	if (t->profile & BA_TRANSPORT_PROFILE_A2DP_SINK)
		return ba_transport_pcm_start(&t->a2dp.pcm, a2dp_flac_dec_thread, "ba-a2dp-flac", true);

	g_assert_not_reached();
	return -1;
}
