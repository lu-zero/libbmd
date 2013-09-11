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

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <DeckLinkAPI.h>

#include "libbmd/decklink_internal.h"

extern "C" {
#include "libbmd/decklink.h"
}

DecklinkIterator *decklink_interator_alloc(void)
{
    DecklinkIterator *it = (DecklinkIterator *)calloc(1, sizeof(*it));

    if (it)
        it->it = CreateDeckLinkIteratorInstance();

    return it;
}

void decklink_iterator_free(DecklinkIterator *it)
{
    if (!it)
        return;

    if (it->it)
        it->it->Release();

    free(it);
}

static int enumerate_display(IDeckLinkDisplayModeIterator *dm_it,
                             void *priv, int instance, int io,
                             decklink_enum_cb cb)
{
    IDeckLinkDisplayMode *dm = NULL;

    while (dm_it->Next(&dm) == S_OK) {
        BMDProbeString str;
        BMDTimeValue num;
        BMDTimeScale den;

        if (dm->GetName(&str) == S_OK) {
            dm->GetFrameRate(&num, &den);
            cb(priv, instance, io,
               ToStr(str),
               dm->GetWidth(), dm->GetHeight(),
               num, den,
               dm->GetFieldDominance(), dm->GetFlags());
            FreeStr(str);
        }
        dm->Release();
    }
    return 0;
}

static int enumerate_input(IDeckLink *dl,
                           void *priv, int instance,
                           decklink_enum_cb display_cb)
{
    IDeckLinkInput *in                  = NULL;
    IDeckLinkDisplayModeIterator *dm_it = NULL;
    int ret;

    ret = dl->QueryInterface(IID_IDeckLinkInput, (void**)&in);
    if (ret != S_OK)
        return ret;

    ret = in->GetDisplayModeIterator(&dm_it);
    if (ret != S_OK)
        return ret;

    ret = enumerate_display(dm_it, priv, instance, DECKLINK_INPUT, display_cb);

    dm_it->Release();

    return ret;
}

static int enumerate_output(IDeckLink *dl,
                           void *priv, int instance,
                           decklink_enum_cb display_cb)
{
    IDeckLinkOutput *out                 = NULL;
    IDeckLinkDisplayModeIterator *dm_out = NULL;
    int ret;

    ret = dl->QueryInterface(IID_IDeckLinkOutput, (void**)&out);
    if (ret != S_OK)
        return ret;

    ret = out->GetDisplayModeIterator(&dm_out);
    if (ret != S_OK)
        return ret;

    ret = enumerate_display(dm_out, priv, instance, DECKLINK_OUTPUT, display_cb);

    dm_out->Release();
    out->Release();

    return ret;
}

int decklink_enumerate(DecklinkIterator *it,
                       void *priv,
                       int instance, int io,
                       decklink_enum_cb cb)
{
    IDeckLinkDisplayModeIterator *dm_it = NULL;
    IDeckLinkDisplayMode         *dm    = NULL;
    IDeckLink                    *dl    = NULL;
    int i = 0, j = 0 , k = 0;
    int ret;

    while (it->it->Next(&dl) == S_OK) {
        if (i++ == instance || instance < 0)
            if (io & DECKLINK_INPUT)
                enumerate_input(dl, priv, instance, cb);
            if (io & DECKLINK_OUTPUT)
                enumerate_output(dl, priv, instance, cb);
        dl->Release();
    }

    return 0;
}
