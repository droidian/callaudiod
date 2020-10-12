/*
 * Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "callaudiod-manager"

#include "callaudiod.h"
#include "cad-manager.h"
#include "cad-pulse.h"

#include <gio/gio.h>
#include <glib-unix.h>

typedef struct _CadManager {
    CallAudioDbusCallAudioSkeleton parent;
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

    switch (op->type) {
    case CAD_OPERATION_SELECT_MODE:
        call_audio_dbus_call_audio_complete_select_mode(op->object, op->invocation, op->result);
        break;
    case CAD_OPERATION_ENABLE_SPEAKER:
        call_audio_dbus_call_audio_complete_enable_speaker(op->object, op->invocation, op->result);
        break;
    case CAD_OPERATION_MUTE_MIC:
        call_audio_dbus_call_audio_complete_mute_mic(op->object, op->invocation, op->result);
        break;
    default:
        break;
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

    op = malloc(sizeof(CadOperation));
    op->type = CAD_OPERATION_SELECT_MODE;
    op->object = object;
    op->invocation = invocation;
    op->callback = complete_command_cb;

    g_message("Select mode: %u", mode);
    cad_pulse_select_mode(mode, op);
    return TRUE;
}

static gboolean cad_manager_handle_enable_speaker(CallAudioDbusCallAudio *object,
                                                  GDBusMethodInvocation *invocation,
                                                  gboolean enable)
{
    CadOperation *op;

    op = malloc(sizeof(CadOperation));
    op->type = CAD_OPERATION_ENABLE_SPEAKER;
    op->object = object;
    op->invocation = invocation;
    op->callback = complete_command_cb;

    g_message("Enable speaker: %d", enable);
    cad_pulse_enable_speaker(enable, op);
    return TRUE;
}

static gboolean cad_manager_handle_mute_mic(CallAudioDbusCallAudio *object,
                                            GDBusMethodInvocation *invocation,
                                            gboolean mute)
{
    CadOperation *op;

    op = malloc(sizeof(CadOperation));
    op->type = CAD_OPERATION_MUTE_MIC;
    op->object = object;
    op->invocation = invocation;
    op->callback = complete_command_cb;

    g_message("Mute mic: %d", mute);
    cad_pulse_mute_mic(mute, op);
    return TRUE;
}

static void cad_manager_constructed(GObject *object)
{
    G_OBJECT_CLASS(cad_manager_parent_class)->constructed(object);
}

static void cad_manager_dispose(GObject *object)
{
    G_OBJECT_CLASS(cad_manager_parent_class)->dispose(object);
}

static void cad_manager_call_audio_iface_init(CallAudioDbusCallAudioIface *iface)
{
    iface->handle_select_mode = cad_manager_handle_select_mode;
    iface->handle_enable_speaker = cad_manager_handle_enable_speaker;
    iface->handle_mute_mic = cad_manager_handle_mute_mic;
}

static void cad_manager_class_init(CadManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->constructed = cad_manager_constructed;
    object_class->dispose = cad_manager_dispose;
}

static void cad_manager_init(CadManager *self)
{
}

CadManager *cad_manager_get_default(void)
{
    static CadManager *manager;

    if (manager == NULL) {
        manager = g_object_new(CAD_TYPE_MANAGER, NULL);
        g_object_add_weak_pointer(G_OBJECT(manager), (gpointer *)&manager);
    }

    return manager;
}
