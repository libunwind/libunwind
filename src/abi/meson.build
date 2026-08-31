# ABI generation is only available if we're building shared libraries, since static libraries
# can't be analyzed with abigail.
if get_option('default_library') == 'static'
    subdir_done()
endif

_canonical = ''
if target_machine.system() == 'linux'
    _canonical = archnames[target_machine.cpu_family()] / 'linux-gnu'
    if abi_variant != ''
        _canonical = _canonical / abi_variant
    endif
    _canonical = fs.exists(_canonical) ? _canonical : ''
endif

_abi_cmd = [
    abidw,
    '@INPUT@',
    '--no-show-locs',
    '--no-comp-dir-path',
    '--no-corpus-path',
    '--drop-undefined-syms',
    '--drop-private-types',
    '--out-file',
    '@OUTPUT@',
]
# The checked-in baselines are generated without cxx_exceptions, so when it's
# enabled, additionally suppress reports about the extra _Unwind_* symbols it
# adds -- they're expected, not a regression.
_abi_suppressions = [files('libunwind.supp')]
if support_cxx_exceptions
    _abi_suppressions += files('cxx-exceptions.supp')
endif
_abidiff_args = []
foreach _supp : _abi_suppressions
    _abidiff_args += ['--suppressions', _supp]
endforeach
_abidiff_args += ['-l']
_abidiff_cmd = [abidiff] + _abidiff_args
_abidiffs = []
foreach _lib : _libs
    _abifn = 'lib@0@.abi'.format(_lib.name())
    if get_option('default_library') == 'both'
        _lib = _lib.get_shared_lib()
    endif
    _abi = custom_target(output: _abifn, input: _lib, command: _abi_cmd)
    if _canonical != ''
        if not fs.exists(_canonical / _abifn)
            error(
                'no ABI baseline for @0@ in src/abi/@1@.  Generate one with\n'.format(
                    _lib.name(),
                    _canonical,
                )
                + '  ninja -C <builddir> src/abi/@0@ && cp <builddir>/src/abi/@0@ src/abi/@1@/'.format(
                    _abifn,
                    _canonical,
                ),
            )
        endif
        _baseline = files(_canonical / _abifn)
        # A run_target for the manual `ninja abi-check` workflow ...
        _abidiffs += run_target(
            _abifn + '-check',
            command: [_abidiff_cmd, _baseline, _abi],
        )
        # ... and a test so that `meson test` catches ABI breakage too.  abidiff
        # reads the ELF, so it runs on the build machine even for cross builds.
        test(
            _abifn + '-check',
            abidiff,
            args: _abidiff_args + [_baseline, _abi],
            suite: 'abi',
        )
    endif
endforeach
if _canonical != ''
    alias_target('abi-check', _abidiffs)
endif
