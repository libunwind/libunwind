sh = find_program('/bin/sh', 'sh', native: true)

_check_nms = configure_file(
    output: 'check-namespace.sh',
    input: 'check-namespace.sh.in',
    configuration: {
        'build_arch': archnames.get(
            build_machine.cpu_family(),
            build_machine.cpu_family(),
        ),
        'arch': archnames[target_machine.cpu_family()],
        'target_os': target_machine.system(),
        'CONFIG_WEAK_BACKTRACE_TRUE': get_option('weak_backtrace') ? '' : '# ',
        'enable_cxx_exceptions': support_cxx_exceptions ? 'yes' : 'no',
        'enable_debug_frame': cfg.has('CONFIG_DEBUG_FRAME') ? 'yes' : 'no',
    },
)

_args = cc.get_supported_arguments('-fno-optimize-sibling-calls')
_inc = [internal_inc, include_directories('../src')]
_opts = ['debug=true']
_env = {'UNW_DEBUG_LEVEL': '4'}

if host_machine.system() != 'windows'
    _exe = executable(
        'test-proc-info',
        'test-proc-info.c',
        link_with: libunwind_generic_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'test-strerror',
        'test-strerror.c',
        link_with: libunwind_generic_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)
endif

_test_ptrace = disabler()
if build_ptrace
    _test_ptrace = executable(
        'test-ptrace',
        'test-ptrace.c',
        link_with: [libunwind_ptrace_lib, libunwind_generic_lib],
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
endif

if not unw_remote_only
    test(
        'check-namespace.sh',
        sh,
        args: [_check_nms, '-v'],
        depends: [libunwind_lib, libunwind_generic_lib],
        env: {
            'LIBUNWIND': libunwind_lib.full_path(),
            'LIBUNWIND_GENERIC': libunwind_generic_lib.full_path(),
        },
    )

    _exe = executable(
        'Gtest-bt',
        'Gtest-bt.c',
        'ident.c',
        link_with: [libunwind_generic_lib, libunwind_lib],
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Ltest-bt',
        'Ltest-bt.c',
        'ident.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Gtest-init',
        'Gtest-init.cxx',
        link_with: [libunwind_generic_lib, libunwind_lib],
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Ltest-init',
        'Ltest-init.cxx',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Gtest-concurrent',
        'Gtest-concurrent.c',
        link_with: [libunwind_generic_lib, libunwind_lib],
        include_directories: _inc,
        dependencies: [libc_deps, threads_dep],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Ltest-concurrent',
        'Ltest-concurrent.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps, threads_dep],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Gtest-sig-context',
        'Gtest-sig-context.c',
        link_with: libunwind_generic_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Ltest-sig-context',
        'Ltest-sig-context.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Gtest-trace',
        'Gtest-trace.c',
        'ident.c',
        link_with: [libunwind_generic_lib, libunwind_lib],
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Ltest-trace',
        'Ltest-trace.c',
        'ident.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Ltest-mem-validate',
        'Ltest-mem-validate.c',
        link_with: [libunwind_generic_lib, libunwind_lib],
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'test-async-sig',
        'test-async-sig.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps, threads_dep],
        override_options: _opts,
    )
    test(
        _exe.name(),
        _exe,
        env: _env,
        # https://github.com/libunwind/libunwind/issues/670
        should_fail: target_machine.cpu_family() == 'ppc',
    )

    _exe = executable(
        'test-flush-cache',
        'test-flush-cache.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'test-init-remote',
        'test-init-remote.c',
        link_with: [libunwind_generic_lib, libunwind_lib],
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'test-mem',
        'test-mem.c',
        link_with: [libunwind_generic_lib, libunwind_lib],
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'test-reg-state',
        'test-reg-state.c',
        link_with: [libunwind_generic_lib, libunwind_lib],
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Ltest-varargs',
        'Ltest-varargs.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Ltest-nomalloc',
        'Ltest-nomalloc.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps, dl_dep],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Ltest-nocalloc',
        'Ltest-nocalloc.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps, dl_dep, threads_dep],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Lrs-race',
        'Lrs-race.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps, threads_dep],
        override_options: _opts,
    )
    test(_exe.name(), _exe, env: _env)

    _exe = executable(
        'Gperf-simple',
        'Gperf-simple.c',
        link_with: [libunwind_generic_lib, libunwind_lib],
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    benchmark(_exe.name(), _exe)

    _exe = executable(
        'Lperf-simple',
        'Lperf-simple.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    benchmark(_exe.name(), _exe)

    _exe = executable(
        'Gperf-trace',
        'Gperf-trace.c',
        link_with: [libunwind_generic_lib, libunwind_lib],
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    benchmark(_exe.name(), _exe)

    _exe = executable(
        'Lperf-trace',
        'Lperf-trace.c',
        link_with: libunwind_lib,
        include_directories: _inc,
        dependencies: [libc_deps],
        override_options: _opts,
    )
    benchmark(_exe.name(), _exe)

    if target_machine.cpu_family() == 'ia64'
        _exe = executable(
            'Gia64-test-stack',
            'Gia64-test-stack.c',
            'ia64-test-stack-asm.S',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Lia64-test-stack',
            'Lia64-test-stack.c',
            'ia64-test-stack-asm.S',
            link_with: libunwind_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Gia64-test-nat',
            'Gia64-test-nat.c',
            'ia64-test-nat-asm.S',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Lia64-test-nat',
            'Lia64-test-nat.c',
            'ia64-test-nat-asm.S',
            link_with: libunwind_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Gia64-test-rbs',
            'Gia64-test-rbs.c',
            'ia64-test-rbs-asm.S',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Lia64-test-rbs',
            'Lia64-test-rbs.c',
            'ia64-test-rbs-asm.S',
            link_with: libunwind_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Gia64-test-readonly',
            'Gia64-test-readonly.c',
            'ia64-test-readonly-asm.S',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Lia64-test-readonly',
            'Lia64-test-readonly.c',
            'ia64-test-reaeonly-asm.S',
            link_with: libunwind_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'ia64-test-setjmp',
            'ia64-test-setjmp.c',
            link_with: libunwind_setjmp_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'ia64-test-sig',
            'ia64-test-sig.c',
            link_with: libunwind_generic_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Gtest-dyn1',
            'Gtest-dyn1.c',
            'flush-cache.S',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Ltest-dyn1',
            'Ltest-dyn1.c',
            'flush-cache.S',
            link_with: libunwind_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'ia64-test-dyn1',
            'ia64-test-dyn1.c',
            'ia64-dyn-asm.S',
            'flush-cache.S',
            link_with: libunwind_generic_lib,
            include_directories: _inc,
            dependencies: libc_deps,
            override_options: _opts,
        )
        test('run-ia64-test-dyn1', _test_ptrace, args: ['-t', _exe], env: _env)
    else
        _exe = executable(
            'Ltest-init-local-signal',
            'Ltest-init-local-signal.c',
            'Ltest-init-local-signal-lib.c',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(
            _exe.name(),
            _exe,
            env: _env,
            # https://github.com/libunwind/libunwind/issues/560
            should_fail: target_machine.cpu_family() == 'ppc',
        )
    endif

    if target_machine.cpu_family() == 'ppc64'
        _exe = executable(
            'ppc64-test-plt',
            'ppc64-test-plt.c',
            link_with: [libunwind_generic_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)
    elif target_machine.cpu_family() == 'x86_64'
        _exe = executable(
            'Gx64-test-dwarf-expressions',
            'Gx64-test-dwarf-expressions.c',
            'x64-test-dwarf-expressions.S',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Lx64-test-dwarf-expressions',
            'Lx64-test-dwarf-expressions.c',
            'x64-test-dwarf-expressions.S',
            link_with: libunwind_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'x64-unwind-badjmp-signal-frame',
            'x64-unwind-badjmp-signal-frame.c',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)
    elif target_machine.cpu_family() == 'aarch64'
        _exe = executable(
            'aarch64-test-plt',
            'aarch64-test-plt.c',
            link_with: libunwind_generic_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'aarch64-test-frame-record',
            'aarch64-test-frame-record.c',
            link_with: libunwind_generic_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _args = ['-fno-inline', '-march=armv8-a+sve']
        if cc.has_multi_arguments(_args)
            _exe = executable(
                'Garm64-test-sve-signal',
                'Garm64-test-sve-signal.c',
                c_args: _args,
                link_with: [libunwind_generic_lib, libunwind_lib],
                include_directories: _inc,
                dependencies: [libc_deps],
                override_options: _opts,
            )
            test(_exe.name(), _exe, env: _env)

            _exe = executable(
                'Larm64-test-sve-signal',
                'Larm64-test-sve-signal.c',
                c_args: _args,
                link_with: libunwind_lib,
                include_directories: _inc,
                dependencies: [libc_deps],
                override_options: _opts,
            )
            test(_exe.name(), _exe, env: _env)
        endif
    endif

    if enable_unw_resume_tests
        _exe = executable(
            'Gtest-exc',
            'Gtest-exc.c',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(
            _exe.name(),
            _exe,
            env: _env,
            # https://github.com/libunwind/libunwind/issues/559
            should_fail: target_machine.cpu_family() == 'ppc',
        )

        _exe = executable(
            'Ltest-exc',
            'Ltest-exc.c',
            link_with: libunwind_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(
            _exe.name(),
            _exe,
            env: _env,
            # https://github.com/libunwind/libunwind/issues/559
            should_fail: target_machine.cpu_family() == 'ppc',
        )

        _exe = executable(
            'Gtest-resume-sig',
            'Gtest-resume-sig.c',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Ltest-resume-sig',
            'Ltest-resume-sig.c',
            link_with: libunwind_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Gtest-resume-sig-rt',
            'Gtest-resume-sig-rt.c',
            link_with: [libunwind_generic_lib, libunwind_lib],
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)

        _exe = executable(
            'Ltest-resume-sig-rt',
            'Ltest-resume-sig-rt.c',
            link_with: libunwind_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)
    endif

    if build_ptrace
        test(
            _test_ptrace.name(),
            _test_ptrace,
            # https://github.com/libunwind/libunwind/issues/392
            should_fail: target_machine.cpu_family() == 'x86',
            env: _env,
        )

        # These tests are only valid if PTRACE_SINGLESTEP works
        _should_fail = target_machine.cpu_family() in [
            'mips',
            'mips64',
            'riscv',
            'arm',
            'loongarch64',
        ]

        _exe = executable(
            'mapper',
            'mapper.c',
            include_directories: _inc,
            dependencies: libc_deps,
            override_options: _opts,
        )
        test(
            'run-ptrace-mapper',
            _test_ptrace,
            args: ['-c', '-n', '-t', _exe],
            env: _env,
            should_fail: _should_fail,
        )

        _exe = executable(
            'test-ptrace-misc',
            'test-ptrace-misc.c',
            'ident.c',
            include_directories: _inc,
            dependencies: libc_deps,
            override_options: _opts,
        )
        test(
            'run-ptrace-misc',
            _test_ptrace,
            args: ['-c', '-t', _exe],
            env: _env,
            should_fail: _should_fail,
        )
    endif

    if build_setjmp
        _exe = executable(
            'test-setjmp',
            'test-setjmp.c',
            link_with: libunwind_setjmp_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(_exe.name(), _exe, env: _env)
    endif

    if support_cxx_exceptions
        _exe = executable(
            'Ltest-cxx-exceptions',
            'Ltest-cxx-exceptions.cxx',
            link_with: libunwind_lib,
            include_directories: _inc,
            dependencies: [libc_deps],
            override_options: _opts,
        )
        test(
            _exe.name(),
            _exe,
            # https://github.com/libunwind/libunwind/issues/561
            should_fail: target_machine.cpu_family() == 'ppc',
            env: _env,
        )
    endif

    if target_machine.system() == 'linux' and build_coredump
        _crasher = executable(
            'crasher',
            'crasher.c',
            include_directories: _inc,
            dependencies: libc_deps,
            override_options: _opts,
        )
        _exe = executable(
            'test-coredump-unwind',
            'test-coredump-unwind.c',
            link_with: [libunwind_coredump_lib, libunwind_generic_lib],
            include_directories: _inc,
            dependencies: libc_deps,
            override_options: _opts,
        )
        test(
            'run-coredump-unwind',
            sh,
            args: [files('run-coredump-unwind')],
            depends: [_crasher, _exe],
            workdir: meson.current_build_dir(),
        )
        if lzma_dep.found()
            test(
                'run-coredump-unwind',
                sh,
                args: [files('run-coredump-unwind'), '-minidebuginfo'],
                depends: [_crasher, _exe],
                workdir: meson.current_build_dir(),
            )
        endif
    endif
endif
