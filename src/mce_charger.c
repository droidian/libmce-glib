/*
 * Copyright (C) 2019-2022 Jolla Ltd.
 * Copyright (C) 2019-2022 Slava Monich <slava.monich@jolla.com>
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

#include "mce_charger.h"
#include "mce_proxy.h"
#include "mce_log_p.h"

#include <mce/dbus-names.h>
#include <mce/mode-names.h>

#include <gutil_misc.h>

/* Generated headers */
#include "com.nokia.mce.request.h"
#include "com.nokia.mce.signal.h"

struct mce_charger_priv {
    MceProxy* proxy;
    gulong proxy_valid_id;
    gulong charger_state_ind_id;
};

enum mce_charger_signal {
    SIGNAL_VALID_CHANGED,
    SIGNAL_STATE_CHANGED,
    SIGNAL_COUNT
};

#define SIGNAL_VALID_CHANGED_NAME   "mce-charger-valid-changed"
#define SIGNAL_STATE_CHANGED_NAME   "mce-charger-state-changed"

static guint mce_charger_signals[SIGNAL_COUNT] = { 0 };

typedef GObjectClass MceChargerClass;
G_DEFINE_TYPE(MceCharger, mce_charger, G_TYPE_OBJECT)
#define PARENT_CLASS mce_charger_parent_class
#define MCE_CHARGER_TYPE (mce_charger_get_type())
#define MCE_CHARGER(obj) (G_TYPE_CHECK_INSTANCE_CAST(obj,\
        MCE_CHARGER_TYPE,MceCharger))

/*==========================================================================*
 * Implementation
 *==========================================================================*/

static
void
mce_charger_state_update(
    MceCharger* self,
    const char* value)
{
    MCE_CHARGER_STATE state;
    MceChargerPriv* priv = self->priv;

    if (!g_strcmp0(value, MCE_CHARGER_STATE_ON)) {
        state = MCE_CHARGER_ON;
    } else if (!g_strcmp0(value, MCE_CHARGER_STATE_OFF)) {
        state = MCE_CHARGER_OFF;
    } else {
        GASSERT(!g_strcmp0(value, MCE_CHARGER_STATE_UNKNOWN));
        state = MCE_CHARGER_UNKNOWN;
    }
    if (self->state != state) {
        self->state = state;
        g_signal_emit(self, mce_charger_signals[SIGNAL_STATE_CHANGED], 0);
    }
    if (priv->proxy->valid && !self->valid) {
        self->valid = TRUE;
        g_signal_emit(self, mce_charger_signals[SIGNAL_VALID_CHANGED], 0);
    }
}

static
void
mce_charger_state_query_done(
    GObject* proxy,
    GAsyncResult* result,
    gpointer arg)
{
    GError* error = NULL;
    char* state = NULL;
    MceCharger* self = MCE_CHARGER(arg);

    if (com_nokia_mce_request_call_get_charger_state_finish(
        COM_NOKIA_MCE_REQUEST(proxy), &state, result, &error)) {
        GDEBUG("Charger is currently %s", state);
        mce_charger_state_update(self, state);
        g_free(state);
    } else {
        /*
         * We could retry but it's probably not worth the trouble
         * because the next time charger state changes we receive
         * charger_state_ind signal and sync our state with mce.
         * Until then, this object stays invalid.
         */
        GWARN("Failed to query charger state %s", GERRMSG(error));
        g_error_free(error);
    }
    mce_charger_unref(self);
}

static
void
mce_charger_state_ind(
    ComNokiaMceSignal* proxy,
    const char* state,
    gpointer arg)
{
    GDEBUG("Charger is %s", state);
    mce_charger_state_update(MCE_CHARGER(arg), state);
}

static
void
mce_charger_state_query(
    MceCharger* self)
{
    MceChargerPriv* priv = self->priv;
    MceProxy* proxy = priv->proxy;

    /*
     * proxy->signal and proxy->request may not be available at the
     * time when MceCharger is created. In that case we have to wait
     * for the valid signal before we can connect the charger state
     * signal and submit the initial query.
     */
    if (proxy->signal && !priv->charger_state_ind_id) {
        priv->charger_state_ind_id = g_signal_connect(proxy->signal,
            MCE_CHARGER_STATE_SIG, G_CALLBACK(mce_charger_state_ind), self);
    }
    if (proxy->request && proxy->valid) {
        com_nokia_mce_request_call_get_charger_state(proxy->request, NULL,
            mce_charger_state_query_done, mce_charger_ref(self));
    }
}

static
void
mce_charger_valid_changed(
    MceProxy* proxy,
    void* arg)
{
    MceCharger* self = MCE_CHARGER(arg);

    if (proxy->valid) {
        mce_charger_state_query(self);
    } else {
        if (self->valid) {
            self->valid = FALSE;
            g_signal_emit(self, mce_charger_signals[SIGNAL_VALID_CHANGED], 0);
        }
    }
}

/*==========================================================================*
 * API
 *==========================================================================*/

MceCharger*
mce_charger_new()
{
    /* MCE assumes one charger */
    static MceCharger* mce_charger_instance = NULL;

    if (mce_charger_instance) {
        mce_charger_ref(mce_charger_instance);
    } else {
        mce_charger_instance = g_object_new(MCE_CHARGER_TYPE, NULL);
        mce_charger_state_query(mce_charger_instance);
        g_object_add_weak_pointer(G_OBJECT(mce_charger_instance),
            (gpointer*)(&mce_charger_instance));
    }
    return mce_charger_instance;
}

MceCharger*
mce_charger_ref(
    MceCharger* self)
{
    if (G_LIKELY(self)) {
        g_object_ref(MCE_CHARGER(self));
    }
    return self;
}

void
mce_charger_unref(
    MceCharger* self)
{
    if (G_LIKELY(self)) {
        g_object_unref(MCE_CHARGER(self));
    }
}

gulong
mce_charger_add_valid_changed_handler(
    MceCharger* self,
    MceChargerFunc fn,
    void* arg)
{
    return (G_LIKELY(self) && G_LIKELY(fn)) ? g_signal_connect(self,
        SIGNAL_VALID_CHANGED_NAME, G_CALLBACK(fn), arg) : 0;
}

gulong
mce_charger_add_state_changed_handler(
    MceCharger* self,
    MceChargerFunc fn,
    void* arg)
{
    return (G_LIKELY(self) && G_LIKELY(fn)) ? g_signal_connect(self,
        SIGNAL_STATE_CHANGED_NAME, G_CALLBACK(fn), arg) : 0;
}

void
mce_charger_remove_handler(
    MceCharger* self,
    gulong id)
{
    if (G_LIKELY(self) && G_LIKELY(id)) {
        g_signal_handler_disconnect(self, id);
    }
}

void
mce_charger_remove_handlers(
    MceCharger* self,
    gulong* ids,
    guint count)
{
    gutil_disconnect_handlers(self, ids, count);
}

/*==========================================================================*
 * Internals
 *==========================================================================*/

static
void
mce_charger_init(
    MceCharger* self)
{
    MceChargerPriv* priv = G_TYPE_INSTANCE_GET_PRIVATE(self, MCE_CHARGER_TYPE,
        MceChargerPriv);

    self->priv = priv;
    priv->proxy = mce_proxy_new();
    priv->proxy_valid_id = mce_proxy_add_valid_changed_handler(priv->proxy,
        mce_charger_valid_changed, self);
}

static
void
mce_charger_finalize(
    GObject* object)
{
    MceCharger* self = MCE_CHARGER(object);
    MceChargerPriv* priv = self->priv;

    if (priv->charger_state_ind_id) {
        g_signal_handler_disconnect(priv->proxy->signal,
            priv->charger_state_ind_id);
    }
    mce_proxy_remove_handler(priv->proxy, priv->proxy_valid_id);
    mce_proxy_unref(priv->proxy);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
mce_charger_class_init(
    MceChargerClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = mce_charger_finalize;
    g_type_class_add_private(klass, sizeof(MceChargerPriv));
    mce_charger_signals[SIGNAL_VALID_CHANGED] =
        g_signal_new(SIGNAL_VALID_CHANGED_NAME,
            G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_FIRST,
            0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    mce_charger_signals[SIGNAL_STATE_CHANGED] =
        g_signal_new(SIGNAL_STATE_CHANGED_NAME,
            G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_FIRST,
            0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
