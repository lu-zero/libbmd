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
#ifndef LIBBMD_DECKLINK_INTERNAL_H
#define LIBBMD_DECKLINK_INTERNAL_H

#include <DeckLinkAPI.h>

extern "C" {
#ifdef HAVE_CFSTRING
#include <CoreFoundation/CoreFoundation.h>
typedef CFStringRef BMDProbeString;
#define ToStr(str) CFStringGetCStringPtr(str, kCFStringEncodingMacRoman)
#define FreeStr(str) CFRelease(str)
#else
typedef const char* BMDProbeString;
#define ToStr(str) (str)
#define FreeStr(str) free((void *)str)
#endif
}

struct DecklinkIterator {
    IDeckLinkIterator *it;
};

struct DecklinkCapture {
    IDeckLink                    *dl;
    IDeckLinkInput               *in;
    IDeckLinkDisplayModeIterator *dm_it;
    IDeckLinkDisplayMode         *dm;
    IDeckLinkConfiguration       *conf;
};

#endif // LIBBMD_DECKLINK_INTERNAL_H
