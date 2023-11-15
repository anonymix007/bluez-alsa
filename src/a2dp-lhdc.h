/*
 * BlueALSA - a2dp-ldac.h
 * Copyright (c) 2016-2021 Arkadiusz Bokowy
 * Copyright (c) 2023      anonymix007
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#pragma once
#ifndef BLUEALSA_A2DPLHDC_H_
#define BLUEALSA_A2DPLHDC_H_

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "a2dp.h"

extern struct a2dp_codec a2dp_lhdc_sink;
extern struct a2dp_codec a2dp_lhdc_source;

#endif
