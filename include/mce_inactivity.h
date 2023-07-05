/*
 * Copyright (c) 2016 - 2023 Jolla Ltd.
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

#ifndef MCE_INACTIVITY_H
#define MCE_INACTIVITY_H

#include "mce_types.h"

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct mce_inactivity_priv MceInactivityPriv;

struct mce_inactivity {
    GObject object;
    MceInactivityPriv* priv;
    gboolean valid;
    gboolean status;
}; /* MceInactivity */

typedef void
(*MceInactivityFunc)(
    MceInactivity* inactivity,
    void* arg);

MceInactivity*
mce_inactivity_new(
    void);

MceInactivity*
mce_inactivity_ref(
    MceInactivity* inactivity);

void
mce_inactivity_unref(
    MceInactivity* inactivity);

gulong
mce_inactivity_add_valid_changed_handler(
    MceInactivity* inactivity,
    MceInactivityFunc fn,
    void* arg);

gulong
mce_inactivity_add_status_changed_handler(
    MceInactivity* inactivity,
    MceInactivityFunc fn,
    void* arg);

void
mce_inactivity_remove_handler(
    MceInactivity* inactivity,
    gulong id);

void
mce_inactivity_remove_handlers(
    MceInactivity* inactivity,
    gulong* ids,
    guint count);

#define mce_inactivity_remove_all_handlers(t, ids) \
        mce_inactivity_remove_handlers(t, ids, G_N_ELEMENTS(ids))

G_END_DECLS

#endif /* MCE_INACTIVITY_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
