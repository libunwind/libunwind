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
_abidiff_cmd = [abidiff, '--suppressions', files('libunwind.supp'), '-l']
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
