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
#ifndef LIBBMD_DECKLINK_H
#define LIBBMD_DECKLINK_H

typedef struct DecklinkIterator DecklinkIterator;

typedef struct DecklinkDisplay DecklinkDisplay;

typedef int (*decklink_enum_cb)(void *priv,
                                int instance, int io,
                                const char *name,
                                int width,
                                int height,
                                int64_t timestamp_num,
                                int64_t timestamp_den,
                                int field_dominance,
                                int flags);

DecklinkIterator *decklink_interator_alloc(void);

int decklink_display_enumerate(DecklinkDisplay *disp, void *priv,
                               int instance, int io,
                               decklink_enum_cb display_cb);
enum {
    DECKLINK_INPUT,
    DECKLINK_OUTPUT,
    DECKLINK_ANY,
};

#endif // LIBBMD_DECKLINK_H
