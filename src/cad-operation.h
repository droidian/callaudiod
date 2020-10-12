/*
 * Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "callaudio-dbus.h"
#include <glib-object.h>

typedef enum {
    CAD_OPERATION_SELECT_MODE = 0,
    CAD_OPERATION_ENABLE_SPEAKER,
    CAD_OPERATION_MUTE_MIC,
} CadOperationType;

typedef struct _CadOperation CadOperation;

typedef void (*CadOperationCallback)(CadOperation *op);

struct _CadOperation {
    CadOperationType type;
    CallAudioDbusCallAudio *object;
    GDBusMethodInvocation *invocation;
    CadOperationCallback callback;
    guint result;
};
