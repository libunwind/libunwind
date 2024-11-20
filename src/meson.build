_libunwind_coredump_srcs = files(
    'coredump/_UCD_access_mem.c',
    'coredump/_UCD_accessors.c',
    'coredump/_UCD_corefile_elf.c',
    'coredump/_UCD_create.c',
    'coredump/_UCD_destroy.c',
    'coredump/_UCD_elf_map_image.c',
    'coredump/_UCD_find_proc_info.c',
    'coredump/_UCD_get_elf_filename.c',
    'coredump/_UCD_get_proc_name.c',
    'coredump/_UPT_access_fpreg.c',
    'coredump/_UPT_elf.c',
    'coredump/_UPT_get_dyn_info_list_addr.c',
    'coredump/_UPT_put_unwind_info.c',
    'coredump/_UPT_resume.c',
    'coredump/ucd_file_table.c',
    'mi/init.c',
)

_libunwind_setjmp_srcs = files(
    'mi/init.c',
    'setjmp/longjmp.c',
    'setjmp/siglongjmp.c',
)

_libunwind_srcs_local_unwind = []
if support_cxx_exceptions
    _libunwind_srcs_local_unwind = files(
        'unwind/Backtrace.c',
        'unwind/DeleteException.c',
        'unwind/FindEnclosingFunction.c',
        'unwind/ForcedUnwind.c',
        'unwind/GetBSP.c',
        'unwind/GetCFA.c',
        'unwind/GetDataRelBase.c',
        'unwind/GetGR.c',
        'unwind/GetIP.c',
        'unwind/GetIPInfo.c',
        'unwind/GetLanguageSpecificData.c',
        'unwind/GetRegionStart.c',
        'unwind/GetTextRelBase.c',
        'unwind/RaiseException.c',
        'unwind/Resume.c',
        'unwind/Resume_or_Rethrow.c',
        'unwind/SetGR.c',
        'unwind/SetIP.c',
    )
endif

_libunwind_srcs_os_local = []
if target_machine.system() == 'linux'
    _libunwind_srcs_os = files('dl-iterate-phdr.c', 'os-linux.c')
    if support_cxx_exceptions
        _libunwind_srcs_os_local = files('mi/_ReadSLEB.c', 'mi/_ReadULEB.c')
    endif
    _libunwind_srcs_aarch64_os = files('aarch64/Gos-linux.c')
    _libunwind_srcs_aarch64_os_local = files('aarch64/Los-linux.c')
    _libunwind_srcs_arm_os = files('arm/Gos-linux.c')
    _libunwind_srcs_arm_os_local = files('arm/Los-linux.c')
    _libunwind_srcs_x86_os = files('x86/Gos-linux.c')
    _libunwind_srcs_x86_os_local = files(
        'x86/Los-linux.c',
        'x86/getcontext-linux.S',
    )
    _libunwind_srcs_x86_64_os = files('x86_64/Gos-linux.c')
    _libunwind_srcs_x86_64_os_local = files('x86_64/Los-linux.c')
    _libunwind_coredump_srcs += files(
        'coredump/_UCD_access_reg_linux.c',
        'coredump/_UCD_get_mapinfo_linux.c',
        'coredump/_UCD_get_threadinfo_prstatus.c',
    )
elif target_machine.system() == 'hpux'
    _libunwind_srcs_os = files('os-hpux.c')
elif target_machine.system() == 'freebsd'
    _libunwind_srcs_os = files('os-freebsd.c')
    _libunwind_srcs_aarch64_os = files('aarch64/Gos-freebsd.c')
    _libunwind_srcs_aarch64_os_local = files(
        'aarch64/Los-freebsd.c',
        'aarch64/setcontext.S',
    )
    _libunwind_srcs_arm_os = files('arm/Gos-freebsd.c')
    _libunwind_srcs_arm_os_local = files('arm/Los-freebsd.c')
    _libunwind_srcs_x86_os = files('x86/Gos-freebsd.c')
    _libunwind_srcs_x86_os_local = files(
        'x86/Los-freebsd.c',
        'x86/getcontext-freebsd.S',
    )
    _libunwind_srcs_x86_64_os = files('x86_64/Gos-freebsd.c')
    _libunwind_srcs_x86_64_os_local = files('x86_64/Los-freebsd.c')
    _libunwind_coredump_srcs += files(
        'coredump/_UCD_access_reg_freebsd.c',
        'coredump/_UCD_get_mapinfo_generic.c',
        'coredump/_UCD_get_threadinfo_prstatus.c',
    )
elif target_machine.system() == 'sunos'
    _libunwind_srcs_os = files('os-solaris.c')
    _libunwind_srcs_x86_64_os = files('x86_64/Gos-solaris.c')
    _libunwind_srcs_x86_64_os_local = files('x86_64/Los-solaris.c')
elif target_machine.system() == 'qnx'
    _libunwind_srcs_os = files('os-qnx.c')
    _libunwind_srcs_aarch64_os = files('aarch64/Gos-qnx.c')
    _libunwind_srcs_aarch64_os_local = files('aarch64/Los-qnx.c')
    _libunwind_srcs_arm_os = files('arm/Gos-other.c')
    _libunwind_srcs_arm_os_local = files('arm/Los-other.c')
    _libunwind_srcs_x86_64_os = files('x86_64/Gos-qnx.c')
    _libunwind_srcs_x86_64_os_local = files('x86_64/Los-qnx.c')
    _libunwind_coredump_srcs += files(
        'coredump/_UCD_access_reg_qnx.c',
        'coredump/_UCD_get_mapinfo_qnx.c',
        'coredump/_UCD_get_threadinfo_prstatus.c',
    )
else
    error('Unsupported target system: @0@'.format(target_machine.system()))
endif

if host_machine.system() == 'windows'
    _libunwind_srcs_os = files('remote/win/missing-functions.c')
endif

_libunwind_srcs_common = [
    _libunwind_srcs_os,
    files('mi/flush_cache.c', 'mi/init.c', 'mi/mempool.c', 'mi/strerror.c'),
]
_libunwind_srcs_generic = files(
    'mi/Gaddress_validator.c',
    'mi/Gdestroy_addr_space.c',
    'mi/Gdyn-extract.c',
    'mi/Gdyn-remote.c',
    'mi/Gfind_dynamic_proc_info.c',
    'mi/Gget_accessors.c',
    'mi/Gget_elf_filename.c',
    'mi/Gget_fpreg.c',
    'mi/Gget_proc_info_by_ip.c',
    'mi/Gget_proc_name.c',
    'mi/Gget_reg.c',
    'mi/Gput_dynamic_unwind_info.c',
    'mi/Gset_cache_size.c',
    'mi/Gset_caching_policy.c',
    'mi/Gset_fpreg.c',
    'mi/Gset_iterate_phdr_function.c',
    'mi/Gset_reg.c',
)

_libunwind_srcs_local_nounwind = [
    _libunwind_srcs_os_local,
    files(
        'mi/Laddress_validator.c',
        'mi/Ldestroy_addr_space.c',
        'mi/Ldyn-extract.c',
        'mi/Lfind_dynamic_proc_info.c',
        'mi/Lget_accessors.c',
        'mi/Lget_elf_filename.c',
        'mi/Lget_fpreg.c',
        'mi/Lget_proc_info_by_ip.c',
        'mi/Lget_proc_name.c',
        'mi/Lget_reg.c',
        'mi/Lput_dynamic_unwind_info.c',
        'mi/Lset_cache_size.c',
        'mi/Lset_caching_policy.c',
        'mi/Lset_fpreg.c',
        'mi/Lset_iterate_phdr_function.c',
        'mi/Lset_reg.c',
        'mi/backtrace.c',
        'mi/dyn-cancel.c',
        'mi/dyn-info-list.c',
        'mi/dyn-register.c',
    ),
]
_libunwind_srcs_local = [
    _libunwind_srcs_local_nounwind,
    _libunwind_srcs_local_unwind,
]

if target_machine.cpu_family() == 'aarch64'
    _libunwind_srcs_aarch64_common = [
        _libunwind_srcs_common,
        files('aarch64/is_fpreg.c', 'aarch64/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_aarch64_common,
        _libunwind_srcs_local,
        _libunwind_srcs_aarch64_os_local,
        files(
            'aarch64/Lapply_reg_state.c',
            'aarch64/Lcreate_addr_space.c',
            'aarch64/Lget_proc_info.c',
            'aarch64/Lget_save_loc.c',
            'aarch64/Lglobal.c',
            'aarch64/Linit.c',
            'aarch64/Linit_local.c',
            'aarch64/Linit_remote.c',
            'aarch64/Lis_signal_frame.c',
            'aarch64/Lreg_states_iterate.c',
            'aarch64/Lregs.c',
            'aarch64/Lresume.c',
            'aarch64/Lstash_frame.c',
            'aarch64/Lstep.c',
            'aarch64/Ltrace.c',
            'aarch64/getcontext.S',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_aarch64_common,
        _libunwind_srcs_generic,
        _libunwind_srcs_aarch64_os,
        files(
            'aarch64/Gapply_reg_state.c',
            'aarch64/Gcreate_addr_space.c',
            'aarch64/Gget_proc_info.c',
            'aarch64/Gget_save_loc.c',
            'aarch64/Gglobal.c',
            'aarch64/Ginit.c',
            'aarch64/Ginit_local.c',
            'aarch64/Ginit_remote.c',
            'aarch64/Gis_signal_frame.c',
            'aarch64/Greg_states_iterate.c',
            'aarch64/Gregs.c',
            'aarch64/Gresume.c',
            'aarch64/Gstash_frame.c',
            'aarch64/Gstep.c',
            'aarch64/Gtrace.c',
        ),
    ]
    _libunwind_setjmp_srcs += files('aarch64/longjmp.S', 'aarch64/siglongjmp.S')
elif target_machine.cpu_family() == 'arm'
    _libunwind_srcs_arm_common = [
        _libunwind_srcs_common,
        files('arm/is_fpreg.c', 'arm/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_arm_common,
        _libunwind_srcs_arm_os_local,
        _libunwind_srcs_local,
        files(
            'arm/Lapply_reg_state.c',
            'arm/Lcreate_addr_space.c',
            'arm/Lex_tables.c',
            'arm/Lget_proc_info.c',
            'arm/Lget_save_loc.c',
            'arm/Lglobal.c',
            'arm/Linit.c',
            'arm/Linit_local.c',
            'arm/Linit_remote.c',
            'arm/Lreg_states_iterate.c',
            'arm/Lregs.c',
            'arm/Lresume.c',
            'arm/Lstash_frame.c',
            'arm/Lstep.c',
            'arm/Ltrace.c',
            'arm/getcontext.S',
            'mi/Lis_plt_entry.c',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_arm_common,
        _libunwind_srcs_arm_os,
        _libunwind_srcs_generic,
        files(
            'arm/Gapply_reg_state.c',
            'arm/Gcreate_addr_space.c',
            'arm/Gex_tables.c',
            'arm/Gget_proc_info.c',
            'arm/Gget_save_loc.c',
            'arm/Gglobal.c',
            'arm/Ginit.c',
            'arm/Ginit_local.c',
            'arm/Ginit_remote.c',
            'arm/Greg_states_iterate.c',
            'arm/Gregs.c',
            'arm/Gresume.c',
            'arm/Gstash_frame.c',
            'arm/Gstep.c',
            'arm/Gtrace.c',
            'mi/Gis_plt_entry.c',
        ),
    ]
    _libunwind_setjmp_srcs += files('arm/siglongjmp.S')
elif target_machine.cpu_family() == 'hppa'
    _libunwind_srcs_hppa_common = [
        _libunwind_srcs_common,
        files('hppa/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_hppa_common,
        _libunwind_srcs_local,
        files(
            'hppa/Lapply_reg_state.c',
            'hppa/Lcreate_addr_space.c',
            'hppa/Lget_proc_info.c',
            'hppa/Lget_save_loc.c',
            'hppa/Lglobal.c',
            'hppa/Linit.c',
            'hppa/Linit_local.c',
            'hppa/Linit_remote.c',
            'hppa/Lis_signal_frame.c',
            'hppa/Lreg_states_iterate.c',
            'hppa/Lregs.c',
            'hppa/Lresume.c',
            'hppa/Lstep.c',
            'hppa/getcontext.S',
            'hppa/setcontext.S',
            'mi/Lis_plt_entry.c',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_hppa_common,
        _libunwind_srcs_generic,
        files(
            'hppa/Gapply_reg_state.c',
            'hppa/Gcreate_addr_space.c',
            'hppa/Gget_proc_info.c',
            'hppa/Gget_save_loc.c',
            'hppa/Gglobal.c',
            'hppa/Ginit.c',
            'hppa/Ginit_local.c',
            'hppa/Ginit_remote.c',
            'hppa/Gis_signal_frame.c',
            'hppa/Greg_states_iterate.c',
            'hppa/Gregs.c',
            'hppa/Gresume.c',
            'hppa/Gstep.c',
            'mi/Gis_plt_entry.c',
        ),
    ]
    _libunwind_setjmp_srcs += files('hppa/siglongjmp.S')
elif target_machine.cpu_family() == 'ia64'
    _mk_cursor = find_program(files('ia64/mk_cursor_i'))
    _gcursor_i = custom_target(
        input: 'ia64/mk_Gcursor_i.c',
        output: 'mk_Gcursor_i.s',
        command: [cc, '-S', '@INPUT@', '-o', '@OUTPUT@'],
    )
    _gcursor_i = custom_target(
        input: _gcursor_i,
        output: 'Gcursor_i.h',
        command: [_mk_cursor, '@INPUT@'],
        capture: true,
    )
    _lcursor_i = custom_target(
        input: 'ia64/mk_Lcursor_i.c',
        output: 'mk_Lcursor_i.s',
        command: [cc, '-S', '@INPUT@', '-o', '@OUTPUT@'],
    )
    _lcursor_i = custom_target(
        input: _lcursor_i,
        output: 'Lcursor_i.h',
        command: [_mk_cursor, '@INPUT@'],
        capture: true,
    )
    _libunwind_srcs_ia64_common = [
        _libunwind_srcs_common,
        files('ia64/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_ia64_common,
        _libunwind_srcs_local,
        _lcursor_i,
        files(
            'ia64/Lapply_reg_state.c',
            'ia64/Lcreate_addr_space.c',
            'ia64/Lfind_unwind_table.c',
            'ia64/Lget_proc_info.c',
            'ia64/Lget_save_loc.c',
            'ia64/Lglobal.c',
            'ia64/Linit.c',
            'ia64/Linit_local.c',
            'ia64/Linit_remote.c',
            'ia64/Linstall_cursor.S',
            'ia64/Lis_signal_frame.c',
            'ia64/Lparser.c',
            'ia64/Lrbs.c',
            'ia64/Lreg_states_iterate.c',
            'ia64/Lregs.c',
            'ia64/Lresume.c',
            'ia64/Lscript.c',
            'ia64/Lstep.c',
            'ia64/Ltables.c',
            'ia64/dyn_info_list.S',
            'ia64/getcontext.S',
            'mi/Lis_plt_entry.c',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_ia64_common,
        _libunwind_srcs_generic,
        _gcursor_i,
        files(
            'ia64/Gapply_reg_state.c',
            'ia64/Gcreate_addr_space.c',
            'ia64/Gfind_unwind_table.c',
            'ia64/Gget_proc_info.c',
            'ia64/Gget_save_loc.c',
            'ia64/Gglobal.c',
            'ia64/Ginit.c',
            'ia64/Ginit_local.c',
            'ia64/Ginit_remote.c',
            'ia64/Ginstall_cursor.S',
            'ia64/Gis_signal_frame.c',
            'ia64/Gparser.c',
            'ia64/Grbs.c',
            'ia64/Greg_states_iterate.c',
            'ia64/Gregs.c',
            'ia64/Gresume.c',
            'ia64/Gscript.c',
            'ia64/Gstep.c',
            'ia64/Gtables.c',
            'mi/Gis_plt_entry.c',
        ),
    ]
    _libunwind_setjmp_srcs += files(
        'ia64/longjmp.S',
        'ia64/setjmp.S',
        'ia64/siglongjmp.S',
        'ia64/sigsetjmp.S',
    )
elif target_machine.cpu_family() == 'loongarch64'
    _libunwind_srcs_loongarch64_common = [
        _libunwind_srcs_common,
        files('loongarch64/is_fpreg.c', 'loongarch64/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_loongarch64_common,
        _libunwind_srcs_local,
        files(
            'loongarch64/Lapply_reg_state.c',
            'loongarch64/Lcreate_addr_space.c',
            'loongarch64/Lget_proc_info.c',
            'loongarch64/Lget_save_loc.c',
            'loongarch64/Lglobal.c',
            'loongarch64/Linit.c',
            'loongarch64/Linit_local.c',
            'loongarch64/Linit_remote.c',
            'loongarch64/Lis_signal_frame.c',
            'loongarch64/Lreg_states_iterate.c',
            'loongarch64/Lregs.c',
            'loongarch64/Lresume.c',
            'loongarch64/Lstep.c',
            'loongarch64/getcontext.S',
            'mi/Lis_plt_entry.c',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_loongarch64_common,
        _libunwind_srcs_generic,
        files(
            'loongarch64/Gapply_reg_state.c',
            'loongarch64/Gcreate_addr_space.c',
            'loongarch64/Gget_proc_info.c',
            'loongarch64/Gget_save_loc.c',
            'loongarch64/Gglobal.c',
            'loongarch64/Ginit.c',
            'loongarch64/Ginit_local.c',
            'loongarch64/Ginit_remote.c',
            'loongarch64/Gis_signal_frame.c',
            'loongarch64/Greg_states_iterate.c',
            'loongarch64/Gregs.c',
            'loongarch64/Gresume.c',
            'loongarch64/Gstep.c',
            'mi/Gis_plt_entry.c',
        ),
    ]
    _libunwind_setjmp_srcs += files('loongarch64/siglongjmp.S')
elif target_machine.cpu_family() in ['mips', 'mips64']
    _libunwind_srcs_mips_common = [
        _libunwind_srcs_common,
        files('mips/is_fpreg.c', 'mips/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_mips_common,
        _libunwind_srcs_local,
        files(
            'mi/Lis_plt_entry.c',
            'mips/Lapply_reg_state.c',
            'mips/Lcreate_addr_space.c',
            'mips/Lget_proc_info.c',
            'mips/Lget_save_loc.c',
            'mips/Lglobal.c',
            'mips/Linit.c',
            'mips/Linit_local.c',
            'mips/Linit_remote.c',
            'mips/Lis_signal_frame.c',
            'mips/Lreg_states_iterate.c',
            'mips/Lregs.c',
            'mips/Lresume.c',
            'mips/Lstep.c',
            'mips/getcontext.S',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_mips_common,
        _libunwind_srcs_generic,
        files(
            'mi/Gis_plt_entry.c',
            'mips/Gapply_reg_state.c',
            'mips/Gcreate_addr_space.c',
            'mips/Gget_proc_info.c',
            'mips/Gget_save_loc.c',
            'mips/Gglobal.c',
            'mips/Ginit.c',
            'mips/Ginit_local.c',
            'mips/Ginit_remote.c',
            'mips/Gis_signal_frame.c',
            'mips/Greg_states_iterate.c',
            'mips/Gregs.c',
            'mips/Gresume.c',
            'mips/Gstep.c',
        ),
    ]
    _libunwind_setjmp_srcs += files('mips/siglongjmp.S')
elif target_machine.cpu_family() in ['ppc', 'ppc64']
    _libunwind_srcs_ppc = files(
        'ppc/Lget_proc_info.c',
        'ppc/Lget_save_loc.c',
        'ppc/Linit_local.c',
        'ppc/Linit_remote.c',
        'ppc/Lis_signal_frame.c',
    )
    _libunwind_srcs_ppc_generic = files(
        'ppc/Gget_proc_info.c',
        'ppc/Gget_save_loc.c',
        'ppc/Ginit_local.c',
        'ppc/Ginit_remote.c',
        'ppc/Gis_signal_frame.c',
    )
    if target_machine.cpu_family() == 'ppc'
        _libunwind_srcs_ppc32_common = [
            _libunwind_srcs_common,
            files('ppc32/get_func_addr.c', 'ppc32/is_fpreg.c', 'ppc32/regname.c'),
        ]
        _libunwind_srcs = [
            _libunwind_srcs_ppc32_common,
            _libunwind_srcs_local,
            _libunwind_srcs_ppc,
            files(
                'mi/Lis_plt_entry.c',
                'ppc32/Lapply_reg_state.c',
                'ppc32/Lcreate_addr_space.c',
                'ppc32/Lglobal.c',
                'ppc32/Linit.c',
                'ppc32/Lreg_states_iterate.c',
                'ppc32/Lregs.c',
                'ppc32/Lresume.c',
                'ppc32/Lstep.c',
            ),
        ]
        _libunwind_generic_srcs = [
            _libunwind_srcs_ppc32_common,
            _libunwind_srcs_generic,
            _libunwind_srcs_ppc_generic,
            files(
                'mi/Gis_plt_entry.c',
                'ppc32/Gapply_reg_state.c',
                'ppc32/Gcreate_addr_space.c',
                'ppc32/Gglobal.c',
                'ppc32/Ginit.c',
                'ppc32/Greg_states_iterate.c',
                'ppc32/Gregs.c',
                'ppc32/Gresume.c',
                'ppc32/Gstep.c',
            ),
        ]
        _libunwind_setjmp_srcs += files('ppc/longjmp.S', 'ppc/siglongjmp.S')
    elif target_machine.cpu_family() == 'ppc64'
        _libunwind_srcs_ppc64_common = [
            _libunwind_srcs_common,
            files('ppc64/get_func_addr.c', 'ppc64/is_fpreg.c', 'ppc64/regname.c'),
        ]
        _libunwind_srcs = [
            _libunwind_srcs_ppc64_common,
            _libunwind_srcs_local,
            _libunwind_srcs_ppc,
            files(
                'ppc64/Lapply_reg_state.c',
                'ppc64/Lcreate_addr_space.c',
                'ppc64/Lglobal.c',
                'ppc64/Linit.c',
                'ppc64/Lreg_states_iterate.c',
                'ppc64/Lregs.c',
                'ppc64/Lresume.c',
                'ppc64/Lstep.c',
            ),
        ]
        _libunwind_generic_srcs = [
            _libunwind_srcs_ppc64_common,
            _libunwind_srcs_generic,
            _libunwind_srcs_ppc_generic,
            files(
                'ppc64/Gapply_reg_state.c',
                'ppc64/Gcreate_addr_space.c',
                'ppc64/Gglobal.c',
                'ppc64/Ginit.c',
                'ppc64/Greg_states_iterate.c',
                'ppc64/Gregs.c',
                'ppc64/Gresume.c',
                'ppc64/Gstep.c',
            ),
        ]
        _libunwind_setjmp_srcs += files('ppc/longjmp.S', 'ppc/siglongjmp.S')
    endif
elif target_machine.cpu_family() in ['riscv32', 'riscv64']
    _libunwind_srcs_riscv_common = [
        _libunwind_srcs_common,
        files('riscv/is_fpreg.c', 'riscv/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_riscv_common,
        _libunwind_srcs_local,
        files(
            'mi/Lis_plt_entry.c',
            'riscv/Lapply_reg_state.c',
            'riscv/Lcreate_addr_space.c',
            'riscv/Lget_proc_info.c',
            'riscv/Lget_save_loc.c',
            'riscv/Lglobal.c',
            'riscv/Linit.c',
            'riscv/Linit_local.c',
            'riscv/Linit_remote.c',
            'riscv/Lis_signal_frame.c',
            'riscv/Lreg_states_iterate.c',
            'riscv/Lregs.c',
            'riscv/Lresume.c',
            'riscv/Lstep.c',
            'riscv/getcontext.S',
            'riscv/setcontext.S',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_riscv_common,
        _libunwind_srcs_generic,
        files(
            'mi/Gis_plt_entry.c',
            'riscv/Gapply_reg_state.c',
            'riscv/Gcreate_addr_space.c',
            'riscv/Gget_proc_info.c',
            'riscv/Gget_save_loc.c',
            'riscv/Gglobal.c',
            'riscv/Ginit.c',
            'riscv/Ginit_local.c',
            'riscv/Ginit_remote.c',
            'riscv/Gis_signal_frame.c',
            'riscv/Greg_states_iterate.c',
            'riscv/Gregs.c',
            'riscv/Gresume.c',
            'riscv/Gstep.c',
        ),
    ]
    _libunwind_setjmp_srcs += files('riscv/siglongjmp.S')
elif target_machine.cpu_family() == 's390x'
    _libunwind_srcs_s390x_common = [
        _libunwind_srcs_common,
        files('s390x/is_fpreg.c', 's390x/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_s390x_common,
        _libunwind_srcs_local,
        files(
            'mi/Lis_plt_entry.c',
            's390x/Lapply_reg_state.c',
            's390x/Lcreate_addr_space.c',
            's390x/Lget_proc_info.c',
            's390x/Lget_save_loc.c',
            's390x/Lglobal.c',
            's390x/Linit.c',
            's390x/Linit_local.c',
            's390x/Linit_remote.c',
            's390x/Lis_signal_frame.c',
            's390x/Lreg_states_iterate.c',
            's390x/Lregs.c',
            's390x/Lresume.c',
            's390x/Lstep.c',
            's390x/getcontext.S',
            's390x/setcontext.S',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_s390x_common,
        _libunwind_srcs_generic,
        files(
            'mi/Gis_plt_entry.c',
            's390x/Gapply_reg_state.c',
            's390x/Gcreate_addr_space.c',
            's390x/Gget_proc_info.c',
            's390x/Gget_save_loc.c',
            's390x/Gglobal.c',
            's390x/Ginit.c',
            's390x/Ginit_local.c',
            's390x/Ginit_remote.c',
            's390x/Gis_signal_frame.c',
            's390x/Greg_states_iterate.c',
            's390x/Gregs.c',
            's390x/Gresume.c',
            's390x/Gstep.c',
        ),
    ]
elif target_machine.cpu_family() == 'sh4'
    _libunwind_srcs_sh_common = [
        _libunwind_srcs_common,
        files('sh/is_fpreg.c', 'sh/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_sh_common,
        _libunwind_srcs_local,
        files(
            'mi/Lis_plt_entry.c',
            'sh/Lapply_reg_state.c',
            'sh/Lcreate_addr_space.c',
            'sh/Lget_proc_info.c',
            'sh/Lget_save_loc.c',
            'sh/Lglobal.c',
            'sh/Linit.c',
            'sh/Linit_local.c',
            'sh/Linit_remote.c',
            'sh/Lis_signal_frame.c',
            'sh/Lreg_states_iterate.c',
            'sh/Lregs.c',
            'sh/Lresume.c',
            'sh/Lstep.c',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_sh_common,
        _libunwind_srcs_generic,
        files(
            'mi/Gis_plt_entry.c',
            'sh/Gapply_reg_state.c',
            'sh/Gcreate_addr_space.c',
            'sh/Gget_proc_info.c',
            'sh/Gget_save_loc.c',
            'sh/Gglobal.c',
            'sh/Ginit.c',
            'sh/Ginit_local.c',
            'sh/Ginit_remote.c',
            'sh/Gis_signal_frame.c',
            'sh/Greg_states_iterate.c',
            'sh/Gregs.c',
            'sh/Gresume.c',
            'sh/Gstep.c',
        ),
    ]
    _libunwind_setjmp_srcs += files('sh/siglongjmp.S')
elif target_machine.cpu_family() == 'x86'
    _libunwind_srcs_x86_common = [
        _libunwind_srcs_common,
        files('x86/is_fpreg.c', 'x86/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_x86_common,
        _libunwind_srcs_x86_os_local,
        _libunwind_srcs_local,
        files(
            'mi/Lis_plt_entry.c',
            'x86/Lapply_reg_state.c',
            'x86/Lcreate_addr_space.c',
            'x86/Lget_proc_info.c',
            'x86/Lget_save_loc.c',
            'x86/Lglobal.c',
            'x86/Linit.c',
            'x86/Linit_local.c',
            'x86/Linit_remote.c',
            'x86/Lreg_states_iterate.c',
            'x86/Lregs.c',
            'x86/Lresume.c',
            'x86/Lstep.c',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_x86_common,
        _libunwind_srcs_x86_os,
        _libunwind_srcs_generic,
        files(
            'mi/Gis_plt_entry.c',
            'x86/Gapply_reg_state.c',
            'x86/Gcreate_addr_space.c',
            'x86/Gget_proc_info.c',
            'x86/Gget_save_loc.c',
            'x86/Gglobal.c',
            'x86/Ginit.c',
            'x86/Ginit_local.c',
            'x86/Ginit_remote.c',
            'x86/Greg_states_iterate.c',
            'x86/Gregs.c',
            'x86/Gresume.c',
            'x86/Gstep.c',
        ),
    ]
    _libunwind_setjmp_srcs += files('x86/longjmp.S', 'x86/siglongjmp.S')
elif target_machine.cpu_family() == 'x86_64'
    _libunwind_srcs_x86_64_common = [
        _libunwind_srcs_common,
        files('x86_64/is_fpreg.c', 'x86_64/regname.c'),
    ]
    _libunwind_srcs = [
        _libunwind_srcs_x86_64_common,
        _libunwind_srcs_x86_64_os_local,
        _libunwind_srcs_local,
        files(
            'x86_64/Lapply_reg_state.c',
            'x86_64/Lcreate_addr_space.c',
            'x86_64/Lget_proc_info.c',
            'x86_64/Lget_save_loc.c',
            'x86_64/Lglobal.c',
            'x86_64/Linit.c',
            'x86_64/Linit_local.c',
            'x86_64/Linit_remote.c',
            'x86_64/Lreg_states_iterate.c',
            'x86_64/Lregs.c',
            'x86_64/Lresume.c',
            'x86_64/Lstash_frame.c',
            'x86_64/Lstep.c',
            'x86_64/Ltrace.c',
            'x86_64/getcontext.S',
            'x86_64/setcontext.S',
        ),
    ]
    _libunwind_generic_srcs = [
        _libunwind_srcs_x86_64_common,
        _libunwind_srcs_x86_64_os,
        _libunwind_srcs_generic,
        files(
            'x86_64/Gapply_reg_state.c',
            'x86_64/Gcreate_addr_space.c',
            'x86_64/Gget_proc_info.c',
            'x86_64/Gget_save_loc.c',
            'x86_64/Gglobal.c',
            'x86_64/Ginit.c',
            'x86_64/Ginit_local.c',
            'x86_64/Ginit_remote.c',
            'x86_64/Greg_states_iterate.c',
            'x86_64/Gregs.c',
            'x86_64/Gresume.c',
            'x86_64/Gstash_frame.c',
            'x86_64/Gstep.c',
            'x86_64/Gtrace.c',
        ),
    ]
    _libunwind_setjmp_srcs += files('x86_64/longjmp.S', 'x86_64/siglongjmp.S')
else
    error(
        'Unsupported target architecture: @0@'.format(
            target_machine.cpu_family(),
        ),
    )
endif

libunwind_dwarf_local_lib = []
if target_machine.cpu_family() != 'ia64'  # USE_DWARF
    libunwind_dwarf_common_lib = static_library(
        'unwind-dwarf-common',
        'dwarf/global.c',
        pic: true,
        include_directories: internal_inc,
        dependencies: [libc_deps, lzma_dep, zlib_dep],
    )
    libunwind_dwarf_generic_lib = static_library(
        'unwind-dwarf-generic',
        'dwarf/Gexpr.c',
        'dwarf/Gfde.c',
        'dwarf/Gfind_proc_info-lsb.c',
        'dwarf/Gfind_unwind_table.c',
        'dwarf/Gget_proc_info_in_range.c',
        'dwarf/Gparser.c',
        'dwarf/Gpe.c',
        pic: true,
        include_directories: internal_inc,
        link_with: libunwind_dwarf_common_lib,
        dependencies: [libc_deps, lzma_dep, zlib_dep],
    )
    if not unw_remote_only
        libunwind_dwarf_local_lib = static_library(
            'unwind-dwarf-local',
            'dwarf/Lexpr.c',
            'dwarf/Lfde.c',
            'dwarf/Lfind_proc_info-lsb.c',
            'dwarf/Lfind_unwind_table.c',
            'dwarf/Lget_proc_info_in_range.c',
            'dwarf/Lparser.c',
            'dwarf/Lpe.c',
            pic: true,
            include_directories: internal_inc,
            link_with: libunwind_dwarf_common_lib,
            dependencies: [libc_deps, lzma_dep, zlib_dep],
        )
    endif
endif

if target_machine.cpu_family() in ['arm', 'hppa', 'ppc', 'x86', 'sh4']
    libunwind_elf_lib = static_library(
        'unwind-elf32',
        'elf32.c',
        pic: true,
        include_directories: internal_inc,
        dependencies: [libc_deps, lzma_dep, zlib_dep],
    )
elif target_machine.cpu_family() in [
    'aarch64',
    'ia64',
    'ppc64',
    'x86_64',
    's390x',
]
    libunwind_elf_lib = static_library(
        'unwind-elf64',
        'elf64.c',
        pic: true,
        include_directories: internal_inc,
        dependencies: [libc_deps, lzma_dep, zlib_dep],
    )
elif target_machine.cpu_family() in [
    'mips',
    'mips64',
    'riscv32',
    'riscv64',
    'loongarch64',
]
    libunwind_elf_lib = static_library(
        'unwind-elfxx',
        'elfxx.c',
        pic: true,
        include_directories: internal_inc,
        dependencies: [libc_deps, lzma_dep, zlib_dep],
    )
else
    error(
        'Unsupported target architecture: @0@'.format(
            target_machine.cpu_family(),
        ),
    )
endif

_libs = []

_libunwind_libs = []
if not unw_remote_only
    libunwind_lib = library(
        'unwind',
        _libunwind_srcs,
        include_directories: internal_inc,
        link_whole: [libunwind_dwarf_local_lib, libunwind_elf_lib],
        dependencies: [libc_deps, lzma_dep, zlib_dep],
        version: '8.2.0',
        install: true,
    )
    _libunwind_libs = [libunwind_lib]
    _libs += libunwind_lib
    libunwind_dep = declare_dependency(
        link_with: libunwind_lib,
        include_directories: api_inc,
    )
    meson.override_dependency('libunwind', libunwind_dep)
    pkg.generate(
        libunwind_lib,
        name: 'libunwind',
        description: 'libunwind base library',
    )
endif

libunwind_generic_lib = library(
    'unwind-@0@'.format(archnames[target_machine.cpu_family()]),
    _libunwind_generic_srcs,
    include_directories: internal_inc,
    link_with: [_libunwind_libs],
    link_whole: [libunwind_dwarf_generic_lib, libunwind_elf_lib],
    dependencies: [libc_deps, lzma_dep, zlib_dep],
    version: '8.2.0',
    install: true,
)
_libs += libunwind_generic_lib
libunwind_generic_dep = declare_dependency(
    link_with: libunwind_generic_lib,
    include_directories: api_inc,
)
meson.override_dependency('libunwind-generic', libunwind_generic_dep)
pkg.generate(
    libunwind_generic_lib,
    name: 'libunwind-generic',
    description: 'libunwind generic library',
    requires: _libunwind_libs,
)

if build_coredump
    libunwind_coredump_lib = library(
        'unwind-coredump',
        _libunwind_coredump_srcs,
        include_directories: internal_inc,
        link_with: libunwind_generic_lib,
        dependencies: [libc_deps, lzma_dep, zlib_dep],
        version: '0.0.0',
        install: true,
    )
    _libs += libunwind_coredump_lib
    libunwind_coredump_dep = declare_dependency(
        link_with: libunwind_coredump_lib,
        include_directories: api_inc,
    )
    meson.override_dependency('libunwind-coredump', libunwind_coredump_dep)
    pkg.generate(
        libunwind_coredump_lib,
        name: 'libunwind-coredump',
        description: 'libunwind coredump library',
        requires: [libunwind_generic_lib, _libunwind_libs],
    )
endif

if build_nto
    libunwind_nto_lib = library(
        'unwind-nto',
        'mi/init.c',
        'nto/unw_nto_access_fpreg.c',
        'nto/unw_nto_access_mem.c',
        'nto/unw_nto_accessors.c',
        'nto/unw_nto_access_reg.c',
        'nto/unw_nto_create.c',
        'nto/unw_nto_destroy.c',
        'nto/unw_nto_elf.c',
        'nto/unw_nto_find_proc_info.c',
        'nto/unw_nto_get_dyn_info_list_addr.c',
        'nto/unw_nto_get_proc_name.c',
        'nto/unw_nto_get_elf_filename.c',
        'nto/unw_nto_put_unwind_info.c',
        'nto/unw_nto_resume.c',
        include_directories: internal_inc,
        link_with: [libunwind_generic_lib, _libunwind_libs],
        dependencies: [libc_deps, lzma_dep, zlib_dep],
        install: true,
    )
    _libs += libunwind_nto_lib
    libunwind_nto_dep = declare_dependency(
        link_with: libunwind_nto_lib,
        include_directories: api_inc,
    )
    meson.override_dependency('libunwind-nto', libunwind_nto_dep)
endif

if build_ptrace
    libunwind_ptrace_lib = library(
        'unwind-ptrace',
        'mi/init.c',
        'ptrace/_UPT_access_fpreg.c',
        'ptrace/_UPT_access_mem.c',
        'ptrace/_UPT_access_reg.c',
        'ptrace/_UPT_accessors.c',
        'ptrace/_UPT_create.c',
        'ptrace/_UPT_destroy.c',
        'ptrace/_UPT_elf.c',
        'ptrace/_UPT_find_proc_info.c',
        'ptrace/_UPT_get_dyn_info_list_addr.c',
        'ptrace/_UPT_get_elf_filename.c',
        'ptrace/_UPT_get_proc_name.c',
        'ptrace/_UPT_put_unwind_info.c',
        'ptrace/_UPT_reg_offset.c',
        'ptrace/_UPT_resume.c',
        include_directories: internal_inc,
        link_with: libunwind_generic_lib,
        dependencies: [libc_deps, lzma_dep, zlib_dep],
        version: '0.0.0',
        install: true,
    )
    _libs += libunwind_ptrace_lib
    libunwind_ptrace_dep = declare_dependency(
        link_with: libunwind_ptrace_lib,
        include_directories: api_inc,
    )
    meson.override_dependency('libunwind-ptrace', libunwind_ptrace_dep)
    pkg.generate(
        libunwind_ptrace_lib,
        name: 'libunwind-ptrace',
        description: 'libunwind ptrace library',
        requires: [libunwind_generic_lib, _libunwind_libs],
    )
endif

if build_setjmp
    libunwind_setjmp_lib = library(
        'unwind-setjmp',
        _libunwind_setjmp_srcs,
        include_directories: internal_inc,
        link_with: [libunwind_elf_lib, libunwind_generic_lib, _libunwind_libs],
        version: '0.0.0',
        install: true,
    )
    _libs += libunwind_setjmp_lib
    libunwind_setjmp_dep = declare_dependency(
        link_with: libunwind_setjmp_lib,
        include_directories: api_inc,
    )
    meson.override_dependency('libunwind-setjmp', libunwind_setjmp_dep)
    pkg.generate(
        libunwind_setjmp_lib,
        name: 'libunwind-setjmp',
        description: 'libunwind setjmp library',
        requires: _libunwind_libs,
    )
endif

subdir('abi')
