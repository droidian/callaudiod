/*
 * Copyright (C) 2018, 2019 Purism SPC
 * Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "callaudiod-pulse"

#include "cad-pulse.h"

#include <glib/gi18n.h>
#include <glib-object.h>
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <alsa/use-case.h>

#include <string.h>
#include <stdio.h>

#define APPLICATION_NAME "CallAudio"
#define APPLICATION_ID   "org.mobian-project.CallAudio"

struct _CadPulse
{
    GObject parent_instance;

    pa_glib_mainloop  *loop;
    pa_context        *ctx;

    int card_id;
    int sink_id;
    int source_id;

    gboolean has_voice_profile;
    gchar *speaker_port;
    gchar *earpiece_port;
};

G_DEFINE_TYPE(CadPulse, cad_pulse, G_TYPE_OBJECT);

typedef struct _CadPulseOperation {
    CadPulse *pulse;
    CadOperation *op;
    guint value;
} CadPulseOperation;

#define SINK_CLASS "sound"
#define CARD_BUS_PATH "platform-sound"
#define CARD_FORM_FACTOR "internal"
#define CARD_MODEM_CLASS "modem"

static const gchar *get_available_output(const pa_sink_info *sink, const gchar *exclude)
{
    pa_sink_port_info *available_port = NULL;
    guint i;

    for (i = 0; i < sink->n_ports; i++) {
        pa_sink_port_info *port = sink->ports[i];

        if ((exclude && strcmp(port->name, exclude) == 0) ||
            port->available == PA_PORT_AVAILABLE_NO) {
            continue;
        }

        if (!available_port || port->priority > available_port->priority)
            available_port = port;
    }

    if (available_port)
        return available_port->name;

    return NULL;
}

static void process_new_source(CadPulse *self, const pa_source_info *info)
{
    const gchar *prop;

    prop = pa_proplist_gets(info->proplist, PA_PROP_DEVICE_CLASS);
    if (prop && strcmp(prop, SINK_CLASS) != 0)
        return;
    if (info->card != self->card_id || self->source_id != -1)
        return;

    self->source_id = info->index;

    g_debug("SOURCE: idx=%u name='%s'", info->index, info->name);
}

static void process_sink_ports(CadPulse *self, const pa_sink_info *info)
{
    int i;

    for (i = 0; i < info->n_ports; i++) {
        pa_sink_port_info *port = info->ports[i];

        if (strstr(port->name, SND_USE_CASE_DEV_SPEAKER) != NULL) {
            if (self->speaker_port && strcmp(port->name, self->speaker_port) != 0) {
                g_free(self->speaker_port);
                self->speaker_port = g_strdup(port->name);
            } else if (!self->speaker_port) {
                self->speaker_port = g_strdup(port->name);
            }
        } else if (strstr(port->name, SND_USE_CASE_DEV_HANDSET) != NULL ||
                   strstr(port->name, SND_USE_CASE_DEV_EARPIECE) != NULL) {
            if (self->earpiece_port && strcmp(port->name, self->earpiece_port) != 0) {
                g_free(self->earpiece_port);
                self->earpiece_port = g_strdup(port->name);
            } else if (!self->earpiece_port) {
                self->earpiece_port = g_strdup(port->name);
            }
        }
    }

    g_debug("SINK:   speaker_port='%s'", self->speaker_port);
    if (self->earpiece_port)
        g_debug("SINK:   earpiece_port='%s'", self->earpiece_port);
}

static void process_new_sink(CadPulse *self, const pa_sink_info *info)
{
    const gchar *prop;

    prop = pa_proplist_gets(info->proplist, PA_PROP_DEVICE_CLASS);
    if (prop && strcmp(prop, SINK_CLASS) != 0)
        return;
    if (info->card != self->card_id || self->sink_id != -1)
        return;

    self->sink_id = info->index;

    g_debug("SINK: idx=%u name='%s'", info->index, info->name);

    process_sink_ports(self, info);
}

static void init_source_info(pa_context *ctx, const pa_source_info *info, int eol, void *data)
{
    CadPulse *self = data;

    if (eol == 1)
        return;

    if (!info)
        g_error("PA returned no source info (eol=%d)", eol);

    process_new_source(self, info);
}

static void init_sink_info(pa_context *ctx, const pa_sink_info *info, int eol, void *data)
{
    CadPulse *self = data;

    if (eol == 1)
        return;

    if (!info)
        g_error("PA returned no sink info (eol=%d)", eol);

    process_new_sink(self, info);
}

static void init_card_info(pa_context *ctx, const pa_card_info *info, int eol, void *data)
{
    CadPulse *self = data;
    const gchar *prop;
    guint i;

    if (eol == 1)
        return;

    if (!info)
        g_error("PA returned no card info (eol=%d)", eol);

    prop = pa_proplist_gets(info->proplist, PA_PROP_DEVICE_BUS_PATH);
    if (prop && strcmp(prop, CARD_BUS_PATH) != 0)
        return;
    prop = pa_proplist_gets(info->proplist, PA_PROP_DEVICE_FORM_FACTOR);
    if (prop && strcmp(prop, CARD_FORM_FACTOR) != 0)
        return;
    prop = pa_proplist_gets(info->proplist, PA_PROP_DEVICE_CLASS);
    if (prop && strcmp(prop, CARD_MODEM_CLASS) == 0)
        return;

    self->card_id = info->index;

    g_debug("CARD: idx=%u name='%s'", info->index, info->name);

    for (i = 0; i < info->n_profiles; i++) {
        pa_card_profile_info2 *profile = info->profiles2[i];

        if (strstr(profile->name, SND_USE_CASE_VERB_VOICECALL) != NULL) {
            self->has_voice_profile = TRUE;
            break;
        }
    }
}

static void init_cards_list(CadPulse *self)
{
    pa_operation *op;

    self->card_id = self->sink_id = self->source_id = -1;

    op = pa_context_get_card_info_list(self->ctx, init_card_info, self);
    pa_operation_unref(op);
    op = pa_context_get_sink_info_list(self->ctx, init_sink_info, self);
    pa_operation_unref(op);
    op = pa_context_get_source_info_list(self->ctx, init_source_info, self);
    pa_operation_unref(op);
}

static void changed_cb(pa_context *ctx, pa_subscription_event_type_t type, uint32_t idx, void *data)
{
    CadPulse *self = data;
    pa_subscription_event_type_t kind = type & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
    pa_operation *op = NULL;

    switch (type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
    case PA_SUBSCRIPTION_EVENT_SINK:
        if (idx == self->sink_id && kind == PA_SUBSCRIPTION_EVENT_REMOVE) {
            self->sink_id = -1;
        } else if (kind == PA_SUBSCRIPTION_EVENT_NEW) {
            op = pa_context_get_sink_info_by_index(ctx, idx, init_sink_info, self);
            pa_operation_unref(op);
        }
        break;
    case PA_SUBSCRIPTION_EVENT_SOURCE:
        if (idx == self->source_id && kind == PA_SUBSCRIPTION_EVENT_REMOVE) {
            self->source_id = -1;
        } else if (kind == PA_SUBSCRIPTION_EVENT_NEW) {
            op = pa_context_get_source_info_by_index(ctx, idx, init_source_info, self);
            pa_operation_unref(op);
        }
        break;
    default:
        break;
    }
}

static void subscribe_cb(pa_context *ctx, int success, void *data)
{
    g_debug("subscribe returned %d", success);
}

static void pulse_state_cb(pa_context *ctx, void *data)
{
    CadPulse *self = data;
    pa_context_state_t state;

    state = pa_context_get_state(ctx);
    switch (state) {
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
        g_debug("PA not ready");
        break;
    case PA_CONTEXT_FAILED:
        g_error("Error in PulseAudio context: %s", pa_strerror(pa_context_errno(ctx)));
        break;
    case PA_CONTEXT_TERMINATED:
    case PA_CONTEXT_READY:
        pa_context_set_state_callback(ctx, NULL, NULL);
        pa_context_set_subscribe_callback(ctx, changed_cb, self);
        pa_context_subscribe(ctx,
                             PA_SUBSCRIPTION_MASK_SINK  | PA_SUBSCRIPTION_MASK_SOURCE,
                             subscribe_cb, self);
        init_cards_list(self);
        break;
    }
}

static void constructed(GObject *object)
{
    GObjectClass *parent_class = g_type_class_peek(G_TYPE_OBJECT);
    CadPulse *self = CAD_PULSE(object);
    pa_proplist *props;
    int err;

    /* Meta data */
    props = pa_proplist_new();
    g_assert(props != NULL);

    err = pa_proplist_sets(props, PA_PROP_APPLICATION_NAME, APPLICATION_NAME);
    err = pa_proplist_sets(props, PA_PROP_APPLICATION_ID, APPLICATION_ID);

    self->loop = pa_glib_mainloop_new(NULL);
    if (!self->loop)
        g_error ("Error creating PulseAudio main loop");

    self->ctx = pa_context_new(pa_glib_mainloop_get_api(self->loop), APPLICATION_NAME);
    if (!self->ctx)
        g_error ("Error creating PulseAudio context");

    pa_context_set_state_callback(self->ctx, (pa_context_notify_cb_t)pulse_state_cb, self);
    err = pa_context_connect(self->ctx, NULL, PA_CONTEXT_NOFAIL, 0);
    if (err < 0)
        g_error ("Error connecting to PulseAudio context: %s", pa_strerror(err));

    parent_class->constructed(object);
}


static void dispose(GObject *object)
{
    GObjectClass *parent_class = g_type_class_peek(G_TYPE_OBJECT);
    CadPulse *self = CAD_PULSE(object);

    if (self->speaker_port)
        g_free(self->speaker_port);

    if (self->ctx) {
        pa_context_disconnect(self->ctx);
        pa_context_unref(self->ctx);
        self->ctx = NULL;

        pa_glib_mainloop_free(self->loop);
        self->loop = NULL;
    }

    parent_class->dispose(object);
}

static void cad_pulse_class_init(CadPulseClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->constructed = constructed;
    object_class->dispose = dispose;
}

static void cad_pulse_init(CadPulse *self)
{
}

CadPulse *cad_pulse_get_default(void)
{
    static CadPulse *pulse = NULL;

    if (pulse == NULL) {
        pulse = g_object_new(CAD_TYPE_PULSE, NULL);
        g_object_add_weak_pointer(G_OBJECT(pulse), (gpointer *)&pulse);
    }

    return pulse;
}

static void operation_complete_cb(pa_context *ctx, int success, void *data)
{
    CadPulseOperation *operation = data;

    g_debug("operation returned %d", success);
    operation->op->result = success;
    operation->op->callback(operation->op);

    free(operation);
}

static void set_card_profile(pa_context *ctx, const pa_card_info *info, int eol, void *data)
{
    CadPulseOperation *operation = data;
    pa_card_profile_info2 *profile;
    pa_operation *op = NULL;

    if (eol == 1)
        return;

    if (!info)
        g_error("PA returned no card info (eol=%d)", eol);

    if (info->index != operation->pulse->card_id)
        return;

    profile = info->active_profile2;

    if (strcmp(profile->name, SND_USE_CASE_VERB_VOICECALL) == 0 && operation->value == 0) {
        op = pa_context_set_card_profile_by_index(ctx, operation->pulse->card_id,
                                                  SND_USE_CASE_VERB_HIFI,
                                                  operation_complete_cb, operation);
    } else if (strcmp(profile->name, SND_USE_CASE_VERB_HIFI) == 0 && operation->value == 1) {
        op = pa_context_set_card_profile_by_index(ctx, operation->pulse->card_id,
                                                  SND_USE_CASE_VERB_VOICECALL,
                                                  operation_complete_cb, operation);
    }

    if (op)
        pa_operation_unref(op);
    else
        operation_complete_cb(ctx, 1, operation);
}

static void set_output_port(pa_context *ctx, const pa_sink_info *info, int eol, void *data)
{
    CadPulseOperation *operation = data;
    pa_sink_port_info *port;
    pa_operation *op = NULL;
    const gchar *target_port;

    if (eol == 1)
        return;

    if (!info)
        g_error("PA returned no sink info (eol=%d)", eol);

    if (info->card != operation->pulse->card_id || info->index != operation->pulse->sink_id)
        return;

    if (operation->op->type == CAD_OPERATION_SELECT_MODE)
        target_port = get_available_output(info, operation->pulse->speaker_port);
    else
        target_port = operation->pulse->speaker_port;

    port = info->active_port;

    if (strcmp(port->name, target_port) == 0 && !operation->value) {
        const gchar *available_port = get_available_output(info, target_port);

        if (available_port) {
            op = pa_context_set_sink_port_by_index(ctx, operation->pulse->sink_id,
                                                   available_port,
                                                   operation_complete_cb, operation);
        }
    } else if (strcmp(port->name, target_port) != 0 && operation->value) {
        op = pa_context_set_sink_port_by_index(ctx, operation->pulse->sink_id,
                                               target_port,
                                               operation_complete_cb, operation);
    }

    if (op)
        pa_operation_unref(op);
    else
        operation_complete_cb(ctx, 1, operation);
}

void cad_pulse_select_mode(guint mode, CadOperation *cad_op)
{
    CadPulseOperation *operation = malloc(sizeof(CadPulseOperation));
    pa_operation *op = NULL;

    operation->pulse = cad_pulse_get_default();
    operation->op = cad_op;
    operation->value = mode;

    if (operation->pulse->has_voice_profile) {
        op = pa_context_get_card_info_by_index(operation->pulse->ctx,
                                               operation->pulse->card_id,
                                               set_card_profile, operation);
    } else if (operation->pulse->earpiece_port) {
        op = pa_context_get_sink_info_by_index(operation->pulse->ctx,
                                               operation->pulse->sink_id,
                                               set_output_port, operation);
    } else {
        g_critical("Card %u has no voice call profile and no earpiece port",
                   operation->pulse->card_id);
        operation_complete_cb(operation->pulse->ctx, 0, operation);
    }

    if (op)
        pa_operation_unref(op);
}

void cad_pulse_enable_speaker(gboolean enable, CadOperation *cad_op)
{
    CadPulseOperation *operation;
    pa_operation *op = NULL;

    operation = malloc(sizeof(CadPulseOperation));
    operation->pulse = cad_pulse_get_default();

    if (operation->pulse->sink_id < 0) {
        cad_op->result = 0;
        cad_op->callback(cad_op);
        free(operation);
        return;
    }

    operation->op = cad_op;
    operation->value = (guint)enable;

    op = pa_context_get_sink_info_by_index(operation->pulse->ctx,
                                           operation->pulse->sink_id,
                                           set_output_port, operation);
    pa_operation_unref(op);
}

static void set_mic_mute(pa_context *ctx, const pa_source_info *info, int eol, void *data)
{
    CadPulseOperation *operation = data;
    pa_operation *op = NULL;

    if (eol == 1)
        return;

    if (!info)
        g_error("PA returned no source info (eol=%d)", eol);

    if (info->card != operation->pulse->card_id || info->index != operation->pulse->source_id)
        return;

    if (info->mute && !operation->value) {
        op = pa_context_set_source_mute_by_index(ctx, operation->pulse->source_id, 0,
                                                 operation_complete_cb, operation);
    } else if (!info->mute && operation->value) {
        op = pa_context_set_source_mute_by_index(ctx, operation->pulse->source_id, 1,
                                                 operation_complete_cb, operation);
    }

    if (op)
        pa_operation_unref(op);
    else
        operation_complete_cb(ctx, 1, operation);
}

void cad_pulse_mute_mic(gboolean mute, CadOperation *cad_op)
{
    CadPulseOperation *operation;
    pa_operation *op = NULL;

    operation = malloc(sizeof(CadPulseOperation));
    operation->pulse = cad_pulse_get_default();

    if (operation->pulse->source_id < 0) {
        cad_op->result = 0;
        cad_op->callback(cad_op);
        free(operation);
        return;
    }

    operation->op = cad_op;
    operation->value = (guint)mute;

    op = pa_context_get_source_info_by_index(operation->pulse->ctx,
                                             operation->pulse->source_id,
                                             set_mic_mute, operation);
    pa_operation_unref(op);
}
