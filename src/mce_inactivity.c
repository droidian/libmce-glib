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

#include "mce_inactivity.h"
#include "mce_proxy.h"
#include "mce_log_p.h"

#include <mce/dbus-names.h>
#include <mce/mode-names.h>

#include <gutil_misc.h>

/* Generated headers */
#include "com.nokia.mce.request.h"
#include "com.nokia.mce.signal.h"

struct mce_inactivity_priv {
    MceProxy* proxy;
    gulong proxy_valid_id;
    gulong inactivity_status_ind_id;
};

enum mce_inactivity_signal {
    SIGNAL_VALID_CHANGED,
    SIGNAL_STATUS_CHANGED,
    SIGNAL_COUNT
};

#define SIGNAL_VALID_CHANGED_NAME  "mce-inactivity-valid-changed"
#define SIGNAL_STATUS_CHANGED_NAME "mce-inactivity-mode-changed"

static guint mce_inactivity_signals[SIGNAL_COUNT] = { 0 };

typedef GObjectClass MceInactivityClass;
G_DEFINE_TYPE(MceInactivity, mce_inactivity, G_TYPE_OBJECT)
#define PARENT_CLASS mce_inactivity_parent_class
#define MCE_INACTIVITY_TYPE (mce_inactivity_get_type())
#define MCE_INACTIVITY(obj) (G_TYPE_CHECK_INSTANCE_CAST(obj,\
        MCE_INACTIVITY_TYPE,MceInactivity))

/*==========================================================================*
 * Implementation
 *==========================================================================*/

static
void
mce_inactivity_status_update(
    MceInactivity* self,
    gboolean status)
{
    MceInactivityPriv* priv = self->priv;
    const gboolean prev_status = self->status;
    self->status = status;
    if (self->status != prev_status) {
        g_signal_emit(self, mce_inactivity_signals[SIGNAL_STATUS_CHANGED], 0);
    }
    if (priv->proxy->valid && !self->valid) {
        self->valid = TRUE;
        g_signal_emit(self, mce_inactivity_signals[SIGNAL_VALID_CHANGED], 0);
    }
}

static
void
mce_inactivity_status_query_done(
    GObject* proxy,
    GAsyncResult* result,
    gpointer arg)
{
    GError* error = NULL;
    gboolean status = FALSE;
    MceInactivity* self = MCE_INACTIVITY(arg);

    if (com_nokia_mce_request_call_get_inactivity_status_finish(
        COM_NOKIA_MCE_REQUEST(proxy), &status, result, &error)) {
        GDEBUG("inactivlty is currently %s", status ? "true" : "false");
        mce_inactivity_status_update(self, status);
    } else {
        /*
         * We could retry but it's probably not worth the trouble.
         * There is signal broadcast on mce startup / when inactivity
         * state changes.
         * Until then, this object stays invalid.
         */
        GWARN("Failed to query inactivity status %s", GERRMSG(error));
        g_error_free(error);
    }
    mce_inactivity_unref(self);
}

static
void
mce_inactivity_status_ind(
    ComNokiaMceSignal* proxy,
    gboolean status,
    gpointer arg)
{
    GDEBUG("status is %s", status ? "true" : "false");
    mce_inactivity_status_update(MCE_INACTIVITY(arg), status);
}

static
void
mce_inactivity_status_query(
    MceInactivity* self)
{
    MceInactivityPriv* priv = self->priv;
    MceProxy* proxy = priv->proxy;

    /*
     * proxy->signal and proxy->request may not be available at the
     * time when MceInactivity is created. In that case we have to wait
     * for the valid signal before we can connect the inactivity status
     * signal and submit the initial query.
     */
    if (proxy->signal && !priv->inactivity_status_ind_id) {
        priv->inactivity_status_ind_id = g_signal_connect(proxy->signal,
            MCE_INACTIVITY_SIG, G_CALLBACK(mce_inactivity_status_ind), self);
    }
    if (proxy->request && proxy->valid) {
        com_nokia_mce_request_call_get_inactivity_status(proxy->request, NULL,
            mce_inactivity_status_query_done, mce_inactivity_ref(self));
    }
}

static
void
mce_inactivity_valid_changed(
    MceProxy* proxy,
    void* arg)
{
    MceInactivity* self = MCE_INACTIVITY(arg);

    if (proxy->valid) {
        mce_inactivity_status_query(self);
    } else {
        if (self->valid) {
            self->valid = FALSE;
            g_signal_emit(self, mce_inactivity_signals[SIGNAL_VALID_CHANGED], 0);
        }
    }
}

/*==========================================================================*
 * API
 *==========================================================================*/

MceInactivity*
mce_inactivity_new()
{
    static MceInactivity* mce_inactivity_instance = NULL;

    if (mce_inactivity_instance) {
        mce_inactivity_ref(mce_inactivity_instance);
    } else {
        mce_inactivity_instance = g_object_new(MCE_INACTIVITY_TYPE, NULL);
        mce_inactivity_status_query(mce_inactivity_instance);
        g_object_add_weak_pointer(G_OBJECT(mce_inactivity_instance),
            (gpointer*)(&mce_inactivity_instance));
    }
    return mce_inactivity_instance;
}

MceInactivity*
mce_inactivity_ref(
    MceInactivity* self)
{
    if (G_LIKELY(self)) {
        g_object_ref(MCE_INACTIVITY(self));
    }
    return self;
}

void
mce_inactivity_unref(
    MceInactivity* self)
{
    if (G_LIKELY(self)) {
        g_object_unref(MCE_INACTIVITY(self));
    }
}

gulong
mce_inactivity_add_valid_changed_handler(
    MceInactivity* self,
    MceInactivityFunc fn,
    void* arg)
{
    return (G_LIKELY(self) && G_LIKELY(fn)) ? g_signal_connect(self,
        SIGNAL_VALID_CHANGED_NAME, G_CALLBACK(fn), arg) : 0;
}

gulong
mce_inactivity_add_status_changed_handler(
    MceInactivity* self,
    MceInactivityFunc fn,
    void* arg)
{
    return (G_LIKELY(self) && G_LIKELY(fn)) ? g_signal_connect(self,
        SIGNAL_STATUS_CHANGED_NAME, G_CALLBACK(fn), arg) : 0;
}

void
mce_inactivity_remove_handler(
    MceInactivity* self,
    gulong id)
{
    if (G_LIKELY(self) && G_LIKELY(id)) {
        g_signal_handler_disconnect(self, id);
    }
}

void
mce_inactivity_remove_handlers(
    MceInactivity* self,
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
mce_inactivity_init(
    MceInactivity* self)
{
    MceInactivityPriv* priv = G_TYPE_INSTANCE_GET_PRIVATE(self, MCE_INACTIVITY_TYPE,
        MceInactivityPriv);

    self->priv = priv;
    self->status = FALSE;
    priv->proxy = mce_proxy_new();
    priv->proxy_valid_id = mce_proxy_add_valid_changed_handler(priv->proxy,
        mce_inactivity_valid_changed, self);
}

static
void
mce_inactivity_finalize(
    GObject* object)
{
    MceInactivity* self = MCE_INACTIVITY(object);
    MceInactivityPriv* priv = self->priv;

    if (priv->inactivity_status_ind_id) {
        g_signal_handler_disconnect(priv->proxy->signal,
            priv->inactivity_status_ind_id);
    }
    mce_proxy_remove_handler(priv->proxy, priv->proxy_valid_id);
    mce_proxy_unref(priv->proxy);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
mce_inactivity_class_init(
    MceInactivityClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = mce_inactivity_finalize;
    g_type_class_add_private(klass, sizeof(MceInactivityPriv));
    mce_inactivity_signals[SIGNAL_VALID_CHANGED] =
        g_signal_new(SIGNAL_VALID_CHANGED_NAME,
            G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_FIRST,
            0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    mce_inactivity_signals[SIGNAL_STATUS_CHANGED] =
        g_signal_new(SIGNAL_STATUS_CHANGED_NAME,
            G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_FIRST,
            0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

/*
 * Local Variables:
 * status: C
 * c-basic-offset: 4
 * indent-tabs-status: nil
 * End:
 */
