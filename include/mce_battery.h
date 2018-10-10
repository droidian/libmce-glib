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

#ifndef MCE_BATTERY_H
#define MCE_BATTERY_H

#include "mce_types.h"

G_BEGIN_DECLS

typedef enum mce_battery_status {
    MCE_BATTERY_UNKNOWN,
    MCE_BATTERY_EMPTY,
    MCE_BATTERY_LOW,
    MCE_BATTERY_OK,
    MCE_BATTERY_FULL
} MCE_BATTERY_STATUS;

typedef struct mce_battery_priv MceBatteryPriv;

typedef struct mce_battery {
    GObject object;
    MceBatteryPriv* priv;
    gboolean valid;
    guint level;
    MCE_BATTERY_STATUS status;
} MceBattery;

typedef void
(*MceBatteryFunc)(
    MceBattery* battery,
    void* arg);

MceBattery*
mce_battery_new(
    void);

MceBattery*
mce_battery_ref(
    MceBattery* battery);

void
mce_battery_unref(
    MceBattery* battery);

gulong
mce_battery_add_valid_changed_handler(
    MceBattery* battery,
    MceBatteryFunc fn,
    void* arg);

gulong
mce_battery_add_level_changed_handler(
    MceBattery* battery,
    MceBatteryFunc fn,
    void* arg);

gulong
mce_battery_add_status_changed_handler(
    MceBattery* battery,
    MceBatteryFunc fn,
    void* arg);

void
mce_battery_remove_handler(
    MceBattery* battery,
    gulong id);

void
mce_battery_remove_handlers(
    MceBattery* battery,
    gulong *ids,
    guint count);

#define mce_battery_remove_all_handlers(d, ids) \
    mce_battery_remove_handlers(d, ids, G_N_ELEMENTS(ids))

G_END_DECLS

#endif /* MCE_BATTERY_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
