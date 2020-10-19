/*
 * Copyright (C) 2019 Purism SPC
 * Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "libcallaudio.h"

#include <glib.h>

int main (int argc, char *argv[0])
{
    g_autoptr(GOptionContext) opt_context = NULL;
    g_autoptr(GError) err = NULL;
    int mode = -1;
    int speaker = -1;
    int mic = -1;

    const GOptionEntry options [] = {
        {"select-mode", 'm', 0, G_OPTION_ARG_INT, &mode, "Select mode", NULL},
        {"enable-speaker", 's', 0, G_OPTION_ARG_INT, &speaker, "Enable speaker", NULL},
        {"mute-mic", 'u', 0, G_OPTION_ARG_INT, &mic, "Mute microphone", NULL},
        { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };

    opt_context = g_option_context_new("- A helper tool for callaudiod");
    g_option_context_add_main_entries(opt_context, options, NULL);
    if (!g_option_context_parse(opt_context, &argc, &argv, &err)) {
        g_warning("%s", err->message);
        return 1;
    }

    if (!call_audio_init(&err)) {
        g_print ("Failed to init libcallaudio: %s\n", err->message);
        return 1;
    }

    if (mode == CALL_AUDIO_MODE_DEFAULT || mode == CALL_AUDIO_MODE_CALL)
        call_audio_select_mode(mode);

    if (speaker == 0 || speaker == 1)
        call_audio_enable_speaker((gboolean)speaker);

    if (mic == 0 || mic == 1)
        call_audio_mute_mic((gboolean)mic);

    call_audio_deinit ();
    return 0;
}
