configure_file(
    output: 'common.tex',
    input: 'common.tex.in',
    configuration: {'VERSION': meson.project_version()},
)
_texs = files(
    '_U_dyn_cancel.tex',
    '_U_dyn_register.tex',
    'libunwind-coredump.tex',
    'libunwind-dynamic.tex',
    'libunwind-ia64.tex',
    'libunwind-nto.tex',
    'libunwind-ptrace.tex',
    'libunwind-setjmp.tex',
    'libunwind.tex',
    'unw_apply_reg_state.tex',
    'unw_backtrace.tex',
    'unw_create_addr_space.tex',
    'unw_destroy_addr_space.tex',
    'unw_flush_cache.tex',
    'unw_get_accessors.tex',
    'unw_get_elf_filename.tex',
    'unw_get_elf_filename_by_ip.tex',
    'unw_get_fpreg.tex',
    'unw_get_proc_info.tex',
    'unw_get_proc_info_by_ip.tex',
    'unw_get_proc_info_in_range.tex',
    'unw_get_proc_name.tex',
    'unw_get_proc_name_by_ip.tex',
    'unw_get_reg.tex',
    'unw_getcontext.tex',
    'unw_init_local.tex',
    'unw_init_remote.tex',
    'unw_is_fpreg.tex',
    'unw_is_signal_frame.tex',
    'unw_reg_states_iterate.tex',
    'unw_regname.tex',
    'unw_resume.tex',
    'unw_set_cache_size.tex',
    'unw_set_caching_policy.tex',
    'unw_set_fpreg.tex',
    'unw_set_iterate_phdr_function.tex',
    'unw_set_reg.tex',
    'unw_step.tex',
    'unw_strerror.tex',
)

_mans = files('unw_init_local2.3')
foreach _tex : _texs
    if latex2man.found()
        _mans += configure_file(
            output: '@BASENAME@.3',
            input: _tex,
            command: [
                latex2man,
                '-t',
                files('libunwind.trans'),
                '@INPUT@',
                '@OUTPUT@',
            ],
        )
    else
        _mans += files(fs.stem(_tex) + '.3')
    endif
endforeach
install_man(_mans)

_htmls = []
foreach _tex : _texs
    _htmls += custom_target(
        output: '@BASENAME@(3).raw',
        input: _tex,
        command: [
            latex2man,
            '-H',
            '-t',
            files('libunwind.trans'),
            '@INPUT@',
            '@OUTPUT@',
        ],
    )
endforeach
alias_target('html', _htmls)

_pdfs = []
foreach _tex : _texs
    _pdfs += custom_target(
        output: '@BASENAME@(3).pdf',
        input: _tex,
        command: [pdflatex, '@INPUT@', '@OUTPUT@'],
    )
endforeach
alias_target('pdf', _pdfs)
