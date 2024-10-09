_headers = files('libunwind-dynamic.h')

if build_ptrace
    _headers += files('libunwind-ptrace.h')
endif
if build_coredump
    _headers += files('libunwind-coredump.h')
endif
if build_nto
    _headers += files('libunwind-nto.h')
endif
if target_machine.cpu_family() == 'aarch64'
    _headers += files('libunwind-aarch64.h')
elif target_machine.cpu_family() == 'arm'
    _headers += files('libunwind-arm.h')
elif target_machine.cpu_family() == 'ia64'
    _headers += files('libunwind-ia64.h')
elif target_machine.cpu_family() == 'hppa'
    _headers += files('libunwind-hppa.h')
elif target_machine.cpu_family() in ['mips', 'mips64']
    _headers += files('libunwind-mips.h')
elif target_machine.cpu_family() == 'x86'
    _headers += files('libunwind-x86.h')
elif target_machine.cpu_family() == 'x86_64'
    _headers += files('libunwind-x86_64.h')
elif target_machine.cpu_family() == 'ppc'
    _headers += files('libunwind-ppc32.h')
elif target_machine.cpu_family() == 'ppc64'
    _headers += files('libunwind-ppc64.h')
elif target_machine.cpu_family() == 'sh4'
    _headers += files('libunwind-sh.h')
elif target_machine.cpu_family() == 's390x'
    _headers += files('libunwind-s390x.h')
elif target_machine.cpu_family() in ['riscv32', 'riscv64']
    _headers += files('libunwind-riscv.h')
elif target_machine.cpu_family() == 'loongarch64'
    _headers += files('libunwind-loongarch64.h')
endif

_libunwind_h = configure_file(
    output: 'libunwind.h',
    input: 'libunwind.h.in',
    configuration: {'arch': archnames[target_machine.cpu_family()]},
)

if not unw_remote_only
    _headers += _libunwind_h
    if get_option('unwind_header')
        _headers += files('unwind.h')
    endif
endif

ver = meson.project_version().split('.')
_headers += configure_file(
    output: 'libunwind-common.h',
    input: 'libunwind-common.h.in',
    configuration: {
        'PKG_MAJOR': ver[0],
        'PKG_MINOR': ver[1],
        'PKG_EXTRA': ver[2],
    },
    install: true,
    install_dir: get_option('includedir'),
)

install_headers(_headers)
api_inc = include_directories('.')
