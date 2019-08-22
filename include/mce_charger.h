/*
 * Copyright (C) 2019 Jolla Ltd.
 * Copyright (C) 2019 Slava Monich <slava.monich@jolla.com>
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
 */

#ifndef MCE_CHARGER_H
#define MCE_CHARGER_H

#include "mce_types.h"

/* Since 1.0.6 */

G_BEGIN_DECLS

typedef enum mce_charger_state {
    MCE_CHARGER_UNKNOWN,
    MCE_CHARGER_ON,
    MCE_CHARGER_OFF
} MCE_CHARGER_STATE;

typedef struct mce_charger_priv MceChargerPriv;

typedef struct mce_charger {
    GObject object;
    MceChargerPriv* priv;
    gboolean valid;
    MCE_CHARGER_STATE state;
} MceCharger;

typedef void
(*MceChargerFunc)(
    MceCharger* charger,
    void* arg);

MceCharger*
mce_charger_new(
    void);

MceCharger*
mce_charger_ref(
    MceCharger* charger);

void
mce_charger_unref(
    MceCharger* charger);

gulong
mce_charger_add_valid_changed_handler(
    MceCharger* charger,
    MceChargerFunc fn,
    void* arg);

gulong
mce_charger_add_state_changed_handler(
    MceCharger* charger,
    MceChargerFunc fn,
    void* arg);

void
mce_charger_remove_handler(
    MceCharger* charger,
    gulong id);

void
mce_charger_remove_handlers(
    MceCharger* charger,
    gulong *ids,
    guint count);

#define mce_charger_remove_all_handlers(d, ids) \
    mce_charger_remove_handlers(d, ids, G_N_ELEMENTS(ids))

G_END_DECLS

#endif /* MCE_CHARGER_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
