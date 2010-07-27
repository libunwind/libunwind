

set(libunwind_sources_common
    os-linux.c
    mi/init.c
    mi/flush_cache.c
    mi/mempool.c
    mi/strerror.c
    dwarf/global.c
    mi/backtrace.c
    mi/dyn-cancel.c
    mi/dyn-info-list.c
    mi/dyn-register.c
    mi/Ldyn-extract.c
    mi/Lfind_dynamic_proc_info.c
    mi/Lget_accessors.c
    mi/Lget_proc_info_by_ip.c
    mi/Lget_proc_name.c
    mi/Lput_dynamic_unwind_info.c
    mi/Ldestroy_addr_space.c
    mi/Lget_reg.c
    mi/Lset_reg.c
    mi/Lget_fpreg.c
    mi/Lset_fpreg.c
    mi/Lset_caching_policy.c
    mi/_ReadULEB.c
    mi/_ReadSLEB.c
    unwind/Backtrace.c
    unwind/DeleteException.c
    unwind/FindEnclosingFunction.c
    unwind/ForcedUnwind.c
    unwind/GetBSP.c
    unwind/GetCFA.c
    unwind/GetDataRelBase.c
    unwind/GetGR.c
    unwind/GetIP.c
    unwind/GetLanguageSpecificData.c
    unwind/GetRegionStart.c
    unwind/GetTextRelBase.c
    unwind/RaiseException.c
    unwind/Resume.c
    unwind/Resume_or_Rethrow.c
    unwind/SetGR.c
    unwind/SetIP.c
    unwind/GetIPInfo.c
    dwarf/Lexpr.c
    dwarf/Lfde.c
    dwarf/Lparser.c
    dwarf/Lpe.c
    dwarf/Lstep.c
    dwarf/Lfind_proc_info-lsb.c
   )

set(libunwind_sources_x8664_32
    elf32.c
    x86/is_fpreg.c
    x86/regname.c
    x86/setcontext.c
    x86/Lcreate_addr_space.c
    x86/Lget_save_loc.c
    x86/Lglobal.c
    x86/Linit.c
    x86/Linit_local.c
    x86/Linit_remote.c
    x86/Lis_signal_frame.c
    x86/Lget_proc_info.c
    x86/Lregs.c
    x86/Lresume.c
    x86/getcontext.c
    x86/Lstep.c
   )


add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/getcontext.o
                   COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS}
                           -c ${CMAKE_CURRENT_SOURCE_DIR}/x86_64/getcontext.S
                           -o ${CMAKE_CURRENT_BINARY_DIR}/getcontext.o
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/x86_64/getcontext.S)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/setcontext.o
                   COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS}
                           -c ${CMAKE_CURRENT_SOURCE_DIR}/x86_64/setcontext.S
                           -o ${CMAKE_CURRENT_BINARY_DIR}/setcontext.o
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/x86_64/setcontext.S)


set(libunwind_sources_x8664_64
    elf64.c
    x86_64/is_fpreg.c
    x86_64/regname.c
    x86_64/Lcreate_addr_space.c
    x86_64/Lget_save_loc.c
    x86_64/Lglobal.c
    x86_64/Linit.c
    x86_64/Linit_local.c
    x86_64/Linit_remote.c
    x86_64/Lis_signal_frame.c
    x86_64/Lget_proc_info.c
    x86_64/Lregs.c
    x86_64/Lresume.c
    x86_64/Lstep.c
    ${CMAKE_CURRENT_BINARY_DIR}/getcontext.o
    ${CMAKE_CURRENT_BINARY_DIR}/setcontext.o
   )


add_definitions(-DHAVE_CONFIG_H
                -D_GNU_SOURCE
                -fPIC
                -DPIC
               )
        
include_directories(.
                    ../include
                    ${CMAKE_CURRENT_BINARY_DIR}/include
                   )


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libunwind-common.h.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/libunwind-common.h)


# TODO: add checks for all platforms
if("${CMAKE_C_FLAGS}" MATCHES "-m32")
    set(target "x8664_32")
elseif("${CMAKE_C_FLAGS}" MATCHES "-m64")
    set(target "x8664_64")
    include_directories(../include/tdep-x86_64)
else()
    message(FATAL_ERROR "Unknown target")
endif()

add_library(unwind STATIC ${libunwind_sources_common} ${libunwind_sources_${target}})

