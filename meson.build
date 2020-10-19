#
# Copyright (C) 2019 Purism SPC
# Copyright (C) 2020 Arnaud Ferraris <arnaud.ferraris@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

project (
  'callaudiod',
  'c',
  version : '0.0.3',
  license : 'LGPLv3+',
  meson_version : '>= 0.50.0',
  default_options :
    [
      'warning_level=1',
      'buildtype=debugoptimized',
      'c_std=gnu11'
    ],
)

cc = meson.get_compiler('c')

app_name = meson.project_name()

prefix = get_option('prefix')
datadir = get_option('datadir')
sysconfdir = get_option('sysconfdir')
bindir = join_paths(prefix, get_option('bindir'))
libdir = join_paths(prefix, get_option('libdir'))
ifacedir = join_paths(prefix, join_paths(datadir, 'dbus-1', 'interfaces'))
servicedir = join_paths(prefix, join_paths(datadir, 'dbus-1', 'services'))

if datadir.startswith('/')
  full_datadir = datadir
else
  full_datadir = join_paths(prefix, datadir)
endif

if sysconfdir.startswith('/')
  full_sysconfdir = sysconfdir
else
  full_sysconfdir = join_paths(prefix, sysconfdir)
endif

apiversion = '0'
libname = 'libcallaudio-' + apiversion

config_data = configuration_data()
config_data.set_quoted('APP_DATA_NAME', app_name)
config_data.set_quoted('DATADIR', full_datadir)
config_data.set_quoted('SYSCONFDIR', full_sysconfdir)

config_h = configure_file (
    output: 'config.h',
    configuration: config_data
)

subdir('data')
subdir('libcallaudio')
subdir('src')
subdir('tools')
