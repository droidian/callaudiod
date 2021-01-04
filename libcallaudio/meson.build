# Shared library and language bindings

libcallaudio_libdir = get_option('libdir')
libcallaudio_header_dir = get_option('includedir') / libname
libcallaudio_link_args = []
libcallaudio_symbols_file = 'libcallaudio.syms'

# Check linker flags
ld_version_script_arg = '-Wl,--version-script,@0@'.format(meson.current_source_dir() / libcallaudio_symbols_file)
if cc.links('int main() { return 0; }', args : ld_version_script_arg, name : 'ld_supports_version_script')
  libcallaudio_link_args += [ld_version_script_arg]
endif

libcallaudio_headers = [
  'libcallaudio.h',
]

libcallaudio_sources = [
  generated_dbus_sources,
  libcallaudio_headers,
  'libcallaudio.c',
]

libcallaudio_deps = [
  dependency('gobject-2.0'),
  dependency('gio-unix-2.0'),
]

libcallaudio_c_args = [
  '-DG_LOG_DOMAIN="libcallaudio"',
]

libcallaudio_inc = include_directories('.', '../src')

libtype = 'shared_library'
libcallaudio = build_target(
  'callaudio-' + apiversion,
  libcallaudio_sources,
            soversion : 0,
               c_args : libcallaudio_c_args,
  include_directories : libcallaudio_inc,
         dependencies : libcallaudio_deps,
            link_args : libcallaudio_link_args,
              install : true,
          install_dir : libcallaudio_libdir,
          target_type : libtype,
)

libcallaudio_dep = declare_dependency(
  sources : [generated_dbus_sources],
  link_with : libcallaudio,
  dependencies: libcallaudio_deps,
  include_directories : libcallaudio_inc,
)
install_headers(libcallaudio_headers, subdir : libname)

pkgconfig = import('pkgconfig')

pkgconfig.generate(
      libraries : [libcallaudio],
        subdirs : libname,
        version : meson.project_version(),
           name : 'CallAudio',
       filebase : 'libcallaudio-@0@'.format(apiversion),
    description : 'call audio library',
       requires : 'gio-2.0',
    install_dir : join_paths(libcallaudio_libdir, 'pkgconfig'),
  )
