#
# Copyright (C) 2019 Purism SPC
# Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

gnome = import('gnome')

# DBus server interface
prefix = 'org.mobian_project.'
namespace = 'CadDbus'

generated_dbus_sources = []

dbus_interfaces = [ 'org.mobian_project.CallAudio.xml' ]

generated_dbus_sources += gnome.gdbus_codegen('cad-dbus',
    sources          : dbus_interfaces,
    object_manager   : false,
    interface_prefix : prefix,
    install_header   : false,
    namespace        : namespace)

dbus_header_dir = meson.current_build_dir()
dbus_inc = include_directories('.')
install_data(dbus_interfaces, install_dir: ifacedir)

cad_deps = [
    dependency('alsa'),
    dependency('gobject-2.0'),
    dependency('gio-unix-2.0'),
    dependency('libpulse'),
    dependency('libpulse-mainloop-glib'),
]

config_h = configure_file (
    output: 'config.h',
    configuration: config_data
)

executable (
    'callaudiod',
    config_h,
    generated_dbus_sources,
    [
        'callaudiod.c',
        'cad-manager.c', 'cad-manager.h',
        'cad-pulse.c', 'cad-pulse.h',
    ],
    dependencies : cad_deps,
    include_directories : include_directories('..'),
    install : true
)
