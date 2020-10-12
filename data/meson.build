#
# Copyright (C) 2019 Purism SPC
# Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#

gnome = import('gnome')

# DBus server interface
dbus_prefix = 'org.mobian_project.'
dbus_namespace = 'CallAudioDbus'

generated_dbus_sources = []

dbus_interfaces = [ 'org.mobian_project.CallAudio.xml' ]

generated_dbus_sources += gnome.gdbus_codegen('callaudio-dbus',
    sources          : dbus_interfaces,
    object_manager   : false,
    interface_prefix : dbus_prefix,
    install_header   : false,
    namespace        : dbus_namespace)

dbus_header_dir = meson.current_build_dir()
dbus_inc = include_directories('.')
install_data(dbus_interfaces, install_dir: ifacedir)
