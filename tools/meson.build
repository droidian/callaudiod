callaudiocli_sources = [
  'callaudiocli.c',
]

callaudiocli_deps = [
  libcallaudio_dep,
  dependency('gobject-2.0'),
  dependency('gio-unix-2.0'),
]

executable(
  'callaudiocli',
  callaudiocli_sources,
  dependencies: callaudiocli_deps,
  install: true,
)
