/*
 * Blackmagic Devices Decklink C wrapper
 * Copyright (c) 2013 Luca Barbato.
 *
 * This file is part of libbmd.
 *
 * libbmd is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * libbmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libbmd; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef LIBBMD_DECKLINK_CAPTURE_H
#define LIBBMD_DECKLINK_CAPTURE_H

#include <stdint.h>

#include "libbmd/decklink.h"

typedef int (*decklink_video_cb)(void *priv, uint8_t *frame,
                                 int width, int height, int stride,
                                 int64_t timestamp,
                                 int64_t duration, int64_t flags);
typedef int (*decklink_audio_cb)(void *priv, uint8_t *frame,
                                 int nb_samples,
                                 int64_t timestamp, int64_t flags);

/**
 * Main struct assumes you know the video mode you want.
 */
typedef struct DecklinkConf {
    int instance;

    int video_connection;
    int video_mode;
    int pixel_format;
    int field_mode;

    int audio_connection;
    int audio_channels;
    int audio_sample_depth;

    int width, height;
    int64_t tb_den, tb_num;

    void *priv;
    decklink_video_cb video_cb;
    decklink_audio_cb audio_cb;
} DecklinkConf;

typedef struct DecklinkCapture DecklinkCapture;

DecklinkCapture *decklink_capture_alloc(DecklinkIterator *iter,
                                        DecklinkConf *conf);

int decklink_capture_start(DecklinkCapture *capture);

int decklink_capture_stop(DecklinkCapture *capture);

void decklink_capture_free(DecklinkCapture *);

DecklinkDisplay *decklink_capture_display(DecklinkCapture *cap);

#endif // LIBBMD_DECKLINK_CAPTURE_H
