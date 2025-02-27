subdir('api')
subdir('tdep')

internal_inc = [
    include_directories(
        '.',
        'tdep-@0@'.format(archnames[target_machine.cpu_family()]),
    ),
    api_inc,
]

if unw_remote_only and host_machine.system() == 'windows'
    internal_inc += include_directories('remote')
    cfg.set('HAVE_ELF_H', 1)
    cfg.set('HAVE_ENDIAN_H', 1)
    subdir('remote/win')
endif

configure_file(output: 'config.h', configuration: cfg)
# This variable must be set for Autotools-based code to build properly
add_project_arguments('-DHAVE_CONFIG_H', language: ['c', 'cpp'])
