/*
 * Copyright (C) 2020 Purism SPC
 * Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/**
 * CallAudioMode:
 * @CALL_AUDIO_MODE_DEFAULT: Default mode (used for music, alarms, ringtones...)
 * @CALL_AUDIO_MODE_CALL: Voice call mode
 *
 * Enum values to indicate the mode to be selected.
 */

typedef enum _CallAudioMode {
  CALL_AUDIO_MODE_DEFAULT = 0,
  CALL_AUDIO_MODE_CALL,
} CallAudioMode;

typedef void (*CallAudioCallback)(gboolean success,
                                  GError *error,
                                  gpointer data);

gboolean call_audio_init     (GError **error);
gboolean call_audio_is_inited(void);
void     call_audio_deinit   (void);

gboolean call_audio_select_mode      (CallAudioMode mode, GError **error);
gboolean call_audio_select_mode_async(CallAudioMode     mode,
                                      CallAudioCallback cb,
                                      gpointer          data);

gboolean call_audio_enable_speaker      (gboolean enable, GError **error);
gboolean call_audio_enable_speaker_async(gboolean          enable,
                                         CallAudioCallback cb,
                                         gpointer          data);

gboolean call_audio_mute_mic      (gboolean mute, GError **error);
gboolean call_audio_mute_mic_async(gboolean          mute,
                                   CallAudioCallback cb,
                                   gpointer          data);

G_END_DECLS
