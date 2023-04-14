/*
 * Copyright (c) 2023 Jolla Ltd.
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

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include <glib.h>
#include <glib-unix.h>

#include <mce_battery.h>
#include <mce_charger.h>
#include <mce_display.h>
#include <mce_inactivity.h>
#include <mce_tklock.h>

/* ========================================================================= *
 * MAINLOOP
 * ========================================================================= */

static  int        mainloop_result = EXIT_SUCCESS;
static  GMainLoop *mainloop_handle = NULL;

static void mainloop_exit(int exitcode)
{
    if( mainloop_result < exitcode )
        mainloop_result = exitcode;
    if( !mainloop_handle )
        exit(mainloop_result);
    g_main_loop_quit(mainloop_handle);
}

static void mainloop_quit(void)
{
    mainloop_exit(EXIT_SUCCESS);
}

static int mainloop_run(void)
{
    mainloop_handle = g_main_loop_new(NULL, false);
    g_main_loop_run(mainloop_handle);
    g_main_loop_unref(mainloop_handle),
        mainloop_handle = NULL;
    return mainloop_result;
}

/* ========================================================================= *
 * STATUS
 * ========================================================================= */

static const char *bool_repr(bool value)
{
    return value ? "true" : "false";
}

static const char *battery_status_repr(MCE_BATTERY_STATUS status)
{
    static const char * const lut[] = {
        [MCE_BATTERY_UNKNOWN]   = "unknown",
        [MCE_BATTERY_EMPTY]     = "empty",
        [MCE_BATTERY_LOW]       = "low",
        [MCE_BATTERY_OK]        = "ok",
        [MCE_BATTERY_FULL]      = "full",
    };
    return lut[status];
}

static const char *charger_state_repr(MCE_CHARGER_STATE state)
{
    static const char * const lut[] = {
        [MCE_CHARGER_UNKNOWN] = "unknown",
        [MCE_CHARGER_ON]      = "on",
        [MCE_CHARGER_OFF]     = "off",
    };
    return lut[state];
}

static const char *display_state_repr(MCE_DISPLAY_STATE state)
{
    static const char * const lut[] = {
        [MCE_DISPLAY_STATE_OFF] = "off",
        [MCE_DISPLAY_STATE_DIM] = "dim",
        [MCE_DISPLAY_STATE_ON]  = "on",
    };
    return lut[state];
}

static const char *tklock_mode_repr(MCE_TKLOCK_MODE mode)
{
    static const char * const lut[] = {
        [MCE_TKLOCK_MODE_LOCKED]            = "locked",
        [MCE_TKLOCK_MODE_SILENT_LOCKED]     = "silent_locked",
        [MCE_TKLOCK_MODE_LOCKED_DIM]        = "locked_dim",
        [MCE_TKLOCK_MODE_LOCKED_DELAY]      = "locked_delay",
        [MCE_TKLOCK_MODE_SILENT_LOCKED_DIM] = "silent_locked_dim",
        [MCE_TKLOCK_MODE_UNLOCKED]          = "unlocked",
        [MCE_TKLOCK_MODE_SILENT_UNLOCKED]   = "silent_unlocked",
    };
    return lut[mode];
}

static void battery_cb(MceBattery *battery, void *arg)
{
    const char *what_changed = arg;
    printf("battery: valid=%s level=%d status=%s (%s changed)\n",
           bool_repr(battery->valid),
           battery->level,
           battery_status_repr(battery->status),
           what_changed);
}

static void charger_cb(MceCharger *charger, void *arg)
{
    const char *what_changed = arg;
    printf("charger: valid=%s state=%s (%s changed)\n",
           bool_repr(charger->valid),
           charger_state_repr(charger->state),
           what_changed);
}

static void display_cb(MceDisplay *display, void *arg)
{
    const char *what_changed = arg;
    printf("display: valid=%s state=%s (%s changed)\n",
           bool_repr(display->valid),
           display_state_repr(display->state),
           what_changed);
}

static void tklock_cb(MceTklock *tklock, void *arg)
{
    const char *what_changed = arg;
    printf("tklock: valid=%s mode=%s locked=%s (%s changed)\n",
           bool_repr(tklock->valid),
           tklock_mode_repr(tklock->mode),
           bool_repr(tklock->locked),
           what_changed);
}

static void inactivity_cb(MceInactivity *inactivity, void *arg)
{
    const char *what_changed = arg;
    printf("inactivity: valid=%s status=%s (%s changed)\n",
           bool_repr(inactivity->valid),
           bool_repr(inactivity->status),
           what_changed);
}

/* ========================================================================= *
 * MAIN_ENTRY
 * ========================================================================= */

static gboolean quit_cb(gpointer aptr)
{
    printf("quit\n");
    mainloop_quit();
    guint *id_ptr = aptr;
    *id_ptr = 0;
    return G_SOURCE_REMOVE;
}

int
main(int argc, char **argv)
{
    printf("startup\n");

    int exitcode = EXIT_FAILURE;

    MceBattery *battery = mce_battery_new();
    gulong battery_valid_id =
        mce_battery_add_valid_changed_handler(battery, battery_cb, "valid");
    gulong battery_level_id =
        mce_battery_add_level_changed_handler(battery, battery_cb, "level");
    gulong battery_status_id =
        mce_battery_add_status_changed_handler(battery, battery_cb, "status");

    MceCharger *charger = mce_charger_new();
    gulong charger_valid_id =
        mce_charger_add_valid_changed_handler(charger, charger_cb, "valid");
    gulong charger_state_id =
        mce_charger_add_state_changed_handler(charger, charger_cb, "state");

    MceDisplay *display = mce_display_new();
    gulong display_valid_id =
        mce_display_add_valid_changed_handler(display, display_cb, "valid");
    gulong display_state_id =
        mce_display_add_state_changed_handler(display, display_cb, "state");

    MceTklock *tklock = mce_tklock_new();
    gulong tklock_valid_id =
        mce_tklock_add_valid_changed_handler(tklock, tklock_cb, "valid");
    gulong tklock_mode_id =
        mce_tklock_add_mode_changed_handler(tklock, tklock_cb, "mode");
    gulong tklock_locked_id =
        mce_tklock_add_locked_changed_handler(tklock, tklock_cb, "locked");

    MceInactivity *inactivity = mce_inactivity_new();
    gulong inactivity_valid_id =
        mce_inactivity_add_valid_changed_handler(inactivity, inactivity_cb, "valid");
    gulong inactivity_status_id =
        mce_inactivity_add_status_changed_handler(inactivity, inactivity_cb, "status");

    guint timeout_id = 0;
    gint timeout_s = (argc > 1) ? strtol(argv[1], NULL, 0) : 0;
    if( timeout_s > 0)
        timeout_id = g_timeout_add(timeout_s * 1000, quit_cb, &timeout_id);

    guint sigterm_id = g_unix_signal_add(SIGTERM, quit_cb, &sigterm_id);
    guint sigint_id = g_unix_signal_add(SIGINT, quit_cb, &sigint_id);

    printf("mainloop\n");
    exitcode = mainloop_run();
    printf("cleanup\n");

    if( sigterm_id )
        g_source_remove(sigterm_id);
    if( sigint_id )
        g_source_remove(sigint_id);
    if( timeout_id )
        g_source_remove(timeout_id);

    mce_battery_remove_handler(battery, battery_valid_id);
    mce_battery_remove_handler(battery, battery_level_id);
    mce_battery_remove_handler(battery, battery_status_id);
    mce_battery_unref(battery);

    mce_charger_remove_handler(charger, charger_valid_id);
    mce_charger_remove_handler(charger, charger_state_id);
    mce_charger_unref(charger);

    mce_display_remove_handler(display, display_valid_id);
    mce_display_remove_handler(display, display_state_id);
    mce_display_unref(display);

    mce_tklock_remove_handler(tklock, tklock_valid_id);
    mce_tklock_remove_handler(tklock, tklock_mode_id);
    mce_tklock_remove_handler(tklock, tklock_locked_id);
    mce_tklock_unref(tklock);

    mce_inactivity_remove_handler(inactivity, inactivity_valid_id);
    mce_inactivity_remove_handler(inactivity, inactivity_status_id);
    mce_inactivity_unref(inactivity);

    printf("exit\n");
    return exitcode;
}
