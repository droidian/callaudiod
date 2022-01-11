/*
 * Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "callaudiod-manager"

#include "callaudiod.h"
#include "cad-manager.h"
#include "cad-pulse.h"

#include "libcallaudio.h"

#include <gio/gio.h>
#include <glib-unix.h>

typedef struct _CadManager {
    CallAudioDbusCallAudioSkeleton parent;
    CallAudioMode audio_mode;
    CallAudioSpeakerState speaker_state;
    CallAudioMicState mic_state;
} CadManager;

static void cad_manager_call_audio_iface_init(CallAudioDbusCallAudioIface *iface);

G_DEFINE_TYPE_WITH_CODE(CadManager, cad_manager,
                        CALL_AUDIO_DBUS_TYPE_CALL_AUDIO_SKELETON,
                        G_IMPLEMENT_INTERFACE(CALL_AUDIO_DBUS_TYPE_CALL_AUDIO,
                                              cad_manager_call_audio_iface_init));

static void complete_command_cb(CadOperation *op)
{
    if (!op)
        return;

    if (op->success) {
        CadManager *manager = cad_manager_get_default();

        switch (op->type) {
        case CAD_OPERATION_SELECT_MODE:
            CallAudioMode new_audio_mode = GPOINTER_TO_UINT(op->value);
            call_audio_dbus_call_audio_complete_select_mode(op->object, op->invocation, op->success);
            manager->audio_mode = new_audio_mode;
            /* The microphone gets unmuted as well when switching back to the default profile */
            if (new_audio_mode == CALL_AUDIO_MODE_DEFAULT)
                manager->mic_state = CALL_AUDIO_MIC_ON;
            break;
        case CAD_OPERATION_ENABLE_SPEAKER:
            CallAudioSpeakerState new_speaker_state = GPOINTER_TO_UINT(op->value);
            call_audio_dbus_call_audio_complete_enable_speaker(op->object, op->invocation, op->success);
            manager->speaker_state = new_speaker_state;
            break;
        case CAD_OPERATION_MUTE_MIC:
            CallAudioMicState new_mic_state = GPOINTER_TO_UINT(op->value);
            call_audio_dbus_call_audio_complete_mute_mic(op->object, op->invocation, op->success);
            manager->mic_state = new_mic_state;
            break;
        default:
            g_critical("unknown operation %d", op->type);
            break;
        }
    } else {
        g_dbus_method_invocation_return_error(op->invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_FAILED,
                                              "Operation failed");
    }

    free(op);
}

static gboolean cad_manager_handle_select_mode(CallAudioDbusCallAudio *object,
                                               GDBusMethodInvocation *invocation,
                                               guint mode)
{
    CadOperation *op;

    if (mode >= 2) {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_INVALID_ARGS,
                                              "Invalid mode %u", mode);
        return FALSE;
    }

    op = g_new(CadOperation, 1);
    if (!op) {
        g_critical("Unable to allocate memory for select mode operation");
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_NO_MEMORY,
                                              "Failed to allocate operation");
        return FALSE;
    }

    op->type = CAD_OPERATION_SELECT_MODE;
    op->value = GUINT_TO_POINTER(mode);
    op->object = object;
    op->invocation = invocation;
    op->callback = complete_command_cb;

    g_debug("Select mode: %u", mode);
    cad_pulse_select_mode(mode, op);

    return TRUE;
}

static gboolean cad_manager_handle_enable_speaker(CallAudioDbusCallAudio *object,
                                                  GDBusMethodInvocation *invocation,
                                                  gboolean enable)
{
    CadOperation *op;

    op = g_new(CadOperation, 1);
    if (!op) {
        g_critical("Unable to allocate memory for speaker operation");
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_NO_MEMORY,
                                              "Failed to allocate operation");
        return FALSE;
    }

    op->type = CAD_OPERATION_ENABLE_SPEAKER;
    op->value = GUINT_TO_POINTER(enable ? CALL_AUDIO_SPEAKER_ON : CALL_AUDIO_SPEAKER_OFF);
    op->object = object;
    op->invocation = invocation;
    op->callback = complete_command_cb;

    g_debug("Enable speaker: %d", enable);
    cad_pulse_enable_speaker(enable, op);

    return TRUE;
}

static gboolean cad_manager_handle_mute_mic(CallAudioDbusCallAudio *object,
                                            GDBusMethodInvocation *invocation,
                                            gboolean mute)
{
    CadOperation *op;

    op = g_new(CadOperation, 1);
    if (!op) {
        g_critical("Unable to allocate memory for mic operation");
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_NO_MEMORY,
                                              "Failed to allocate operation");
        return FALSE;
    }

    op->type = CAD_OPERATION_MUTE_MIC;
    op->value = GUINT_TO_POINTER(mute ? CALL_AUDIO_MIC_OFF : CALL_AUDIO_MIC_ON);
    op->object = object;
    op->invocation = invocation;
    op->callback = complete_command_cb;

    g_debug("Mute mic: %d", mute);
    cad_pulse_mute_mic(mute, op);

    return TRUE;
}

static void cad_manager_call_audio_iface_init(CallAudioDbusCallAudioIface *iface)
{
    iface->handle_select_mode = cad_manager_handle_select_mode;
    iface->handle_enable_speaker = cad_manager_handle_enable_speaker;
    iface->handle_mute_mic = cad_manager_handle_mute_mic;
}

static void cad_manager_class_init(CadManagerClass *klass)
{
}

static void cad_manager_init(CadManager *self)
{
    self->audio_mode = CALL_AUDIO_MODE_UNKNOWN;
    self->speaker_state = CALL_AUDIO_SPEAKER_UNKNOWN;
    self->mic_state = CALL_AUDIO_MIC_UNKNOWN;
}

CadManager *cad_manager_get_default(void)
{
    static CadManager *manager;

    if (manager == NULL) {
        g_debug("initializing manager...");
        manager = g_object_new(CAD_TYPE_MANAGER, NULL);
        g_object_add_weak_pointer(G_OBJECT(manager), (gpointer *)&manager);
    }

    return manager;
}
