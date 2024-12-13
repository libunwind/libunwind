project(
    'libunwind',
    'c',
    'cpp',
    version: '1.9.0-pre',
    meson_version: '>=0.61.0',
    default_options: ['b_ndebug=if-release', 'c_std=c11', 'cpp_std=c++11'],
)

fs = import('fs')
pkg = import('pkgconfig')
cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

cfg = configuration_data()

# Set basic parameters of the build
cfg.set_quoted('PACKAGE', meson.project_name())
cfg.set_quoted('PACKAGE_NAME', meson.project_name())
cfg.set_quoted('PACKAGE_TARNAME', meson.project_name())
cfg.set_quoted(
    'PACKAGE_BUGREPORT',
    'https://github.com/libunwind/libunwind/issues',
)
cfg.set_quoted('PACKAGE_URL', 'https://github.com/libunwind/libunwind')
cfg.set_quoted(
    'PACKAGE_STRING',
    '@0@ @1@'.format(meson.project_name(), meson.project_version()),
)
cfg.set_quoted('PACKAGE_VERSION', meson.project_version())

# Sizes
cfg.set('SIZEOF_OFF_T', cc.sizeof('off_t', prefix: '#include <sys/types.h>'))

# Header presence checks
if cc.has_header('asm/ptrace_offsets.h')
    cfg.set('HAVE_ASM_PTRACE_OFFSETS_H', 1)
endif
if cc.has_header('asm/ptrace.h')
    cfg.set('HAVE_ASM_PTRACE_H', 1)
endif
if cc.has_header('asm/vsyscall.h')
    cfg.set('HAVE_ASM_VSYSCALL_H', 1)
endif
if cc.has_header('endian.h')
    cfg.set('HAVE_ENDIAN_H', 1)
endif
if cc.has_header('sys/endian.h')
    cfg.set('HAVE_SYS_ENDIAN_H', 1)
endif
if cc.has_header('sys/param.h')
    cfg.set('HAVE_SYS_PARAM_H', 1)
endif
if cc.has_header('execinfo.h')
    cfg.set('HAVE_EXECINFO_H', 1)
endif
if cc.has_header('ia64intrin.h')
    cfg.set('HAVE_IA64INTRIN_H', 1)
endif
if cc.has_header('sys/uc_access.h')
    cfg.set('HAVE_SYS_UC_ACCESS_H', 1)
endif
if cc.has_header('unistd.h')
    cfg.set('HAVE_UNISTD_H', 1)
endif
if cc.has_header('signal.h')
    cfg.set('HAVE_SIGNAL_H', 1)
endif
if cc.has_header('sys/types.h')
    cfg.set('HAVE_SYS_TYPES_H', 1)
endif
if cc.has_header('sys/procfs.h')
    cfg.set('HAVE_SYS_PROCFS_H', 1)
endif
if cc.has_header('sys/ptrace.h')
    cfg.set('HAVE_SYS_PTRACE_H', 1)
endif
if cc.has_header('sys/syscall.h')
    cfg.set('HAVE_SYS_SYSCALL_H', 1)
endif
if cc.has_header('byteswap.h')
    cfg.set('HAVE_BYTESWAP_H', 1)
endif
if cc.has_header('elf.h')
    cfg.set('HAVE_ELF_H', 1)
endif
if cc.has_header('sys/elf.h')
    cfg.set('HAVE_SYS_ELF_H', 1)
endif
if cc.has_header('link.h')
    cfg.set('HAVE_LINK_H', 1)
endif
if cc.has_header('sys/link.h')
    cfg.set('HAVE_SYS_LINK_H', 1)
endif

# Type checks
cfg.set(
    'HAVE_STRUCT_DL_PHDR_INFO_DLPI_SUBS',
    cc.has_member(
        'struct dl_phdr_info',
        'dlpi_subs',
        prefix: '#include <link.h>',
    ) ? 1 : false,
)
if cc.has_type('struct elf_prstatus', prefix: '#include <sys/procfs.h>')
    cfg.set('HAVE_STRUCT_ELF_PRSTATUS', 1)
endif
if cc.has_type('struct prstatus', prefix: '#include <sys/procfs.h>')
    cfg.set('HAVE_STRUCT_PRSTATUS', 1)
endif
if cc.has_type('procfs_status', prefix: '#include <sys/procfs.h>')
    cfg.set('HAVE_PROCFS_STATUS', 1)
endif
if cc.has_type('elf_fpregset_t', prefix: '#include <sys/procfs.h>')
    cfg.set('HAVE_ELF_FPREGSET_T', 1)
endif

# Function checks
if cc.has_function(
    'dl_iterate_phdr',
    args: '-D_GNU_SOURCE',
    prefix: '#include <link.h>',
)
    cfg.set('HAVE_DL_ITERATE_PHDR', 1)
endif
if cc.has_function('dl_phdr_removals_counter')
    cfg.set('HAVE_DL_PHDR_REMOVALS_COUNTER', 1)
endif
if cc.has_function('dlmodinfo')
    cfg.set('HAVE_DLMODINFO', 1)
endif
if cc.has_function('getunwind')
    cfg.set('HAVE_GETUNWIND', 1)
endif
if cc.has_function('ttrace')
    cfg.set('HAVE_TTRACE', 1)
endif
if cc.has_function('mincore', prefix: '#include <sys/mman.h>')
    cfg.set('HAVE_MINCORE', 1)
endif
if cc.has_function('pipe2', prefix: '#include <unistd.h>')
    cfg.set('HAVE_PIPE2', 1)
endif
if cc.has_function('sigaltstack', prefix: '#include <signal.h>')
    cfg.set('HAVE_SIGALTSTACK', 1)
endif
if cc.has_function('execvpe', prefix: '#include <unistd.h>')
    cfg.set('HAVE_EXECVPE', 1)
endif

# Compiler checks
if cc.compiles(
    'int main() { __builtin___clear_cache(0, 0); }',
    name: 'has __builtin___clear_cache',
)
    cfg.set('HAVE__BUILTIN___CLEAR_CACHE', 1)
endif
if cc.compiles(
    'int main() { __builtin_unreachable(); }',
    name: 'has __builtin_unreachable',
)
    cfg.set('HAVE__BUILTIN_UNREACHABLE', 1)
endif

# Enable non-standard C functions
if target_machine.system() in ['linux', 'hpux', 'freebsd', 'qnx']
    add_project_arguments('-D_GNU_SOURCE', language: ['c', 'cpp'])
elif target_machine.system() == 'sunos'
    add_project_arguments('-D__EXTENSIONS__', language: 'c')
endif

# While all builds support a "remote" libunwind (i.e. unwinding a separate process), not all
# systems support a "local" libunwind (i.e. monitoring itself). Determine if the local build is
# possible and if not set UNW_REMOTE_ONLY to optimize away some details that only matter for that.
unw_remote_only = host_machine.cpu_family() != target_machine.cpu_family() or host_machine.system() not in [
    'linux',
    'hpux',
    'freebsd',
    'sunos',
    'qnx',
]
if unw_remote_only
    add_project_arguments('-DUNW_REMOTE_ONLY', language: ['c', 'cpp'])
endif

# Ensure _Unwind_Resume is available in all cases (i.e. as if it came from libc)
libc_deps = []
if not cc.has_function('_Unwind_Resume')
    _found = false
    foreach _libname : ['gcc_s', 'gcc']
        _candidate = cc.find_library(_libname, required: false)
        if cc.has_function('_Unwind_Resume', dependencies: _candidate)
            libc_deps += _candidate
            break
        endif
    endforeach
    if not _found and not unw_remote_only
        error('Local build unavailable, unable to find required _Unwind_Resume')
    endif
endif

# Ensure getcontext is available in all cases (i.e. as if it came from libc)
if not cc.has_function('getcontext')
    libc_deps += dependency('libucontext', required: not unw_remote_only)
endif

# Determine whether to build libunwind-coredump
build_coredump = get_option('coredump').require(
    host_machine.system() != 'windows',
    error_message: 'not supported on windows',
).require(
    host_machine.cpu_family() in [
        'aarch64',
        'arm',
        'mips',
        'mips64',
        'sh4',
        'x86',
        'x86_64',
        'riscv32',
        'riscv64',
        'loongarch64',
    ],
    error_message: 'only supported on certain architectures',
).allowed()

# Determine whether to build libunwind-ptrace
build_ptrace = get_option('ptrace').require(
    cfg.has('HAVE_SYS_PTRACE_H'),
    error_message: 'requires <sys/ptrace.h>',
).allowed()

# Extra constant checks if we're building libunwind-ptrace
if build_ptrace
    if cc.has_header_symbol('sys/ptrace.h', 'PTRACE_POKEUSER')
        cfg.set('HAVE_DECL_PTRACE_POKEUSER', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PTRACE_POKEDATA')
        cfg.set('HAVE_DECL_PTRACE_POKEDATA', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PTRACE_SETREGSET')
        cfg.set('HAVE_DECL_PTRACE_SETREGSET', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PTRACE_TRACEME')
        cfg.set('HAVE_DECL_PTRACE_TRACEME', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PTRACE_CONT')
        cfg.set('HAVE_DECL_PTRACE_CONT', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PTRACE_SINGLESTEP')
        cfg.set('HAVE_DECL_PTRACE_SINGLESTEP', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PTRACE_SYSCALL')
        cfg.set('HAVE_DECL_PTRACE_SYSCALL', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PT_IO')
        cfg.set('HAVE_DECL_PT_IO', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PT_GETREGS')
        cfg.set('HAVE_DECL_PT_GETREGS', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PT_GETFPREGS')
        cfg.set('HAVE_DECL_PT_GETFPREGS', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PT_CONTINUE')
        cfg.set('HAVE_DECL_PT_CONTINUE', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PT_TRACE_ME')
        cfg.set('HAVE_DECL_PT_TRACE_ME', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PT_STEP')
        cfg.set('HAVE_DECL_PT_STEP', 1)
    endif
    if cc.has_header_symbol('sys/ptrace.h', 'PT_SYSCALL')
        cfg.set('HAVE_DECL_PT_SYSCALL', 1)
    endif
endif

# Determine whether we're building libunwind-nto
build_nto = get_option('nto').require(
    cc.has_header('sys/neutrino.h'),
    error_message: 'requires <sys/neutrino.h>',
).allowed()

# Determine whether we're building libunwind-setjmp
build_setjmp = get_option('setjmp').require(
    target_machine.system() != 'qnx',
    error_message: 'not available on qnx',
).disable_auto_if(  # Only enabled by default if local unwinding
    unw_remote_only,
).allowed()

# Determine whether we should support the C++ unwinding interface (_Unwind_*)
support_cxx_exceptions = get_option('cxx_exceptions').require(
    host_machine.cpu_family() in [
        'aarch64',
        'arm',
        'mips',
        'mips64',
        'x86',
        'x86_64',
        's390x',
        'loongarch64',
    ],
    error_message: 'only supported on certain architectures',
).allowed()

# Propagate other options through to the code as #defines
if get_option('weak_backtrace')
    cfg.set('CONFIG_WEAK_BACKTRACE', 1)
endif
if get_option('per_thread_cache')
    cfg.set('HAVE__CACHE_PER_THREAD', 1)
endif
if get_option('debug_frame').require(
    not unw_remote_only,
    error_message: 'only available with local unwinding',
).disable_auto_if(  # Only enabled by default on ARM-based platforms
    host_machine.cpu_family() not in ['arm', 'aarch64'],
).allowed()
    cfg.set('CONFIG_DEBUG_FRAME', 1)
endif
if get_option('block_signals') and host_machine.system() != 'windows'
    cfg.set('CONFIG_BLOCK_SIGNALS', 1)
endif
if get_option('conservative_checks')
    cfg.set('CONSERVATIVE_CHECKS', 1)
endif
if get_option('msabi_support')
    cfg.set('CONFIG_MSABI_SUPPORT', 1)
endif
if get_option('debug_logs')
    add_project_arguments('-DUNW_DEBUG', language: ['c', 'cpp'])
endif

# Extra libraries/tools used for tests
dl_dep = cc.find_library('dl', required: false)
threads_dep = dependency('threads')
abidw = find_program(
    'abidw',
    version: '>=2.0.0',
    native: true,
    required: false,
    disabler: true,
)
abidiff = find_program(
    'abidiff',
    version: '>=2.0.0',
    native: true,
    required: false,
    disabler: true,
)

# Extra libraries/tools used for documentation
latex2man = find_program(
    'latex2man',
    required: get_option('documentation'),
    disabler: true,
    native: true,
)
pdflatex = find_program(
    'pdflatex',
    required: false,
    disabler: true,
    native: true,
)

enable_unw_resume_tests = target_machine.system() != 'qnx'

# Enable support for compressed
lzma_dep = dependency('liblzma', required: get_option('minidebuginfo'))
if lzma_dep.found()
    cfg.set('HAVE_LZMA', 1)
endif
zlib_dep = dependency('zlib', required: get_option('zlibdebuginfo'))
if zlib_dep.found()
    cfg.set('HAVE_ZLIB', 1)
endif

# The names for various architectures differ between Meson's *_machine.cpu_family() and libunwind's
# code. This table converts the former into the latter.
archnames = {
    'aarch64': 'aarch64',
    'arm': 'arm',
    'hppa': 'hppa',  # NB: Not supported by Meson
    'ia64': 'ia64',
    'loongarch64': 'loongarch64',
    'mips': 'mips',
    'mips64': 'mips',
    'ppc': 'ppc32',
    'ppc64': 'ppc64',
    'riscv32': 'riscv',
    'riscv64': 'riscv',
    's390x': 's390x',
    'sh4': 'sh',
    'x86': 'x86',
    'x86_64': 'x86_64',
}

subdir('include')
subdir('doc')
subdir('src')
subdir('tests')
