/*
 * Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "callaudio-dbus.h"

#include "cad-operation.h"
#include <glib-object.h>

#define CALLAUDIO_DBUS_NAME "org.mobian_project.CallAudio"
#define CALLAUDIO_DBUS_PATH "/org/mobian_project/CallAudio"

#define CALLAUDIO_DBUS_TYPE G_BUS_TYPE_SESSION

G_BEGIN_DECLS

#define CAD_TYPE_MANAGER (cad_manager_get_type())

G_DECLARE_FINAL_TYPE(CadManager, cad_manager, CAD, MANAGER,
                     CallAudioDbusCallAudioSkeleton);

CadManager *cad_manager_get_default(void);

G_END_DECLS
