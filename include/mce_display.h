/*
 * Copyright (C) 2016-2022 Jolla Ltd.
 * Copyright (C) 2016-2022 Slava Monich <slava.monich@jolla.com>
 *
 * You may use this file under the terms of BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#ifndef MCE_DISPLAY_H
#define MCE_DISPLAY_H

#include "mce_types.h"

G_BEGIN_DECLS

typedef enum mce_display_state {
    MCE_DISPLAY_STATE_OFF,
    MCE_DISPLAY_STATE_DIM,
    MCE_DISPLAY_STATE_ON
} MCE_DISPLAY_STATE;

typedef struct mce_display_priv MceDisplayPriv;

typedef struct mce_display {
    GObject object;
    MceDisplayPriv* priv;
    gboolean valid;
    MCE_DISPLAY_STATE state;
} MceDisplay;

typedef void
(*MceDisplayFunc)(
    MceDisplay* display,
    void* arg);

MceDisplay*
mce_display_new(
    void);

MceDisplay*
mce_display_ref(
    MceDisplay* display);

void
mce_display_unref(
    MceDisplay* display);

gulong
mce_display_add_valid_changed_handler(
    MceDisplay* display,
    MceDisplayFunc fn,
    void* arg);

gulong
mce_display_add_state_changed_handler(
    MceDisplay* display,
    MceDisplayFunc fn,
    void* arg);

void
mce_display_remove_handler(
    MceDisplay* display,
    gulong id);

void
mce_display_remove_handlers(
    MceDisplay* display,
    gulong *ids,
    guint count);

#define mce_display_remove_all_handlers(d, ids) \
	mce_display_remove_handlers(d, ids, G_N_ELEMENTS(ids))

G_END_DECLS

#endif /* MCE_DISPLAY_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
