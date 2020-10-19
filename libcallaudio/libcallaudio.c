/*
 * Copyright (C) 2020 Purism SPC
 * Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libcallaudio.h"
#include "callaudiod.h"
#include "callaudio-dbus.h"

/**
 * SECTION:libcallaudio
 * @Short_description: Call audio control library
 * @Title: libcallaudio
 *
 * To use the library call #call_audio_init().
 * After initializing the library you can send audio routing requests using the
 * library functions.
 * When your application finishes call #call_audio_deinit() to free resources:
 *
 * |[<!-- language="C" -->
 *    #include <libcallaudio.h>
 *
 *    int main(void)
 *    {
 *       g_autoptr(GError) *err = NULL;
 *       if (call_audio_init(&err)) {
 *         g_error("%s", err->message);
 *       }
 *       ...
 *       call_audio_deinit();
 *       return 0;
 *    }
 * ]|
 */

static CallAudioDbusCallAudio *_proxy;
static gboolean               _initted;

/**
 * call_audio_init:
 * @error: Error information
 *
 * Initialize libcallaudio. This must be called before any other functions.
 *
 * Returns: %TRUE if successful, or %FALSE on error.
 */
gboolean call_audio_init(GError **error)
{
    if (_initted)
        return TRUE;

    _proxy = call_audio_dbus_call_audio_proxy_new_for_bus_sync(
                                    CALLAUDIO_DBUS_TYPE,0, CALLAUDIO_DBUS_NAME,
                                    CALLAUDIO_DBUS_PATH, NULL, error);
    if (!_proxy)
        return FALSE;

    g_object_add_weak_pointer(G_OBJECT(_proxy), (gpointer *)&_proxy);

    _initted = TRUE;
    return TRUE;
}

/**
 * call_audio_deinit:
 *
 * Uninitialize the library when no longer used. Usually called
 * on program shutdown.
 */
void call_audio_deinit(void)
{
    _initted = FALSE;
    g_clear_object(&_proxy);
}

/**
 * call_audio_select_mode:
 * @mode: Audio mode to select
 *
 * Select the audio mode to use.
 *
 * Returns: %TRUE if successful, or %FALSE on error.
 */
gboolean call_audio_select_mode(CallAudioMode mode)
{
    gboolean success = FALSE;
    gboolean ret;
    GError *error = NULL;

    ret = call_audio_dbus_call_audio_call_select_mode_sync(_proxy, mode, &success,
                                                           NULL, &error);
    if (error)
        g_critical("Couldn't set mode %u: %s", mode, error->message);

    g_debug("SelectMode %s: success=%d", ret ? "succeeded" : "failed", success);

    return (ret && success);
}

/**
 * call_audio_enable_speaker:
 * @enable: Desired speaker state
 *
 * Enable or disable speaker output.
 *
 * Returns: %TRUE if successful, or %FALSE on error.
 */
gboolean call_audio_enable_speaker(gboolean enable)
{
    gboolean success;
    gboolean ret;
    GError *error = NULL;

    ret = call_audio_dbus_call_audio_call_enable_speaker_sync(_proxy, enable, &success,
                                                              NULL, &error);
    if (error)
        g_critical("Couldn't enable speaker: %s", error->message);

    g_debug("EnableSpeaker %s: success=%d", ret ? "succeeded" : "failed", success);

    return (ret && success);

}

/**
 * call_audio_mute_mic:
 * @mute: %TRUE to mute the microphone, or %FALSE to unmute it
 *
 * Mute or unmute microphone.
 *
 * Returns: %TRUE if successful, or %FALSE on error.
 */
gboolean call_audio_mute_mic(gboolean mute)
{
    gboolean success = FALSE;
    gboolean ret;
    GError *error = NULL;

    ret = call_audio_dbus_call_audio_call_mute_mic_sync(_proxy, mute, &success,
                                                        NULL, &error);
    if (error)
        g_critical("Couldn't mute mic: %s", error->message);

    g_debug("MuteMic %s: success=%d", ret ? "succeeded" : "failed", success);

    return (ret && success);
}
