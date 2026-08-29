# ABI generation is only available if we're building shared libraries, since static libraries
# can't be analyzed with abigail.
if get_option('default_library') == 'static'
    subdir_done()
endif

_canonical = ''
if target_machine.system() == 'linux'
    _canonical = archnames[target_machine.cpu_family()] / 'linux-gnu'
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
_abidiff_cmd = [abidiff]
foreach _supp : _abi_suppressions
    _abidiff_cmd += ['--suppressions', _supp]
endforeach
_abidiff_cmd += ['-l']
_abidiffs = []
foreach _lib : _libs
    _abifn = 'lib@0@.abi'.format(_lib.name())
    if get_option('default_library') == 'both'
        _lib = _lib.get_shared_lib()
    endif
    _abi = custom_target(output: _abifn, input: _lib, command: _abi_cmd)
    if _canonical != ''
        _abidiffs += run_target(
            _abifn + '-check',
            command: [_abidiff_cmd, files(_canonical / _abifn), _abi],
        )
    endif
endforeach
if _canonical != ''
    alias_target('abi-check', _abidiffs)
endif
