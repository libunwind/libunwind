project(libunwind)

cmake_minimum_required(VERSION 3.16.1)

set(CMAKE_C_STANDARD 11)

set(PKG_MAJOR "1")
set(PKG_MINOR "6")
set(PKG_EXTRA "-rc2")
set(PACKAGE_STRING "libunwind")
set(PACKAGE_BUGREPORT "")

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

if ('$ENV{TARGET}' STREQUAL 'x86_64-linux-gnu')
  set(TARGET_AMD64 1)
  set(arch x86_64)
  add_definitions(-D__x86_64__)
  add_definitions(-D__amd64__)
  add_definitions(-D__linux__)
elseif ('$ENV{TARGET}' STREQUAL 'aarch64-linux-gnu')
  set(TARGET_AARCH64 1)
  set(arch aarch64)
  add_definitions(-D__aarch64__)
  add_definitions(-D__linux__)
elseif ('$ENV{TARGET}' STREQUAL 'arm-linux-gnueabihf')
  set(TARGET_ARM 1)
  set(arch arm)
  add_definitions(-D__arm__)
  add_definitions(-D__linux__)
elseif ('$ENV{TARGET}' STREQUAL 's390x-linux-gnu')
  set(TARGET_S390X 1)
  set(arch s390x)
  add_definitions(-D__s390x__)
  add_definitions(-D__linux__)
elseif ('$ENV{TARGET}' STREQUAL 'loongarch64-linux-gnu')
  set(TARGET_LOONGARCH64 1)
  set(arch loongarch64)
  add_definitions(-D__loongarch64)
  add_definitions(-D__linux__)
else ()
  message(FATAL_ERROR "Unrecognize value in environment variable TARGET: $ENV{TARGET}")
endif ()

include(CheckCSourceCompiles)
include(CheckIncludeFiles)

if ("${CMAKE_GENERATOR}" MATCHES "^Visual Studio.*$")
  message(VERBOSE "Using generator ${CMAKE_GENERATOR}")
  # Assume we are using default MSVC compiler
  add_compile_options(/std:c++latest)
  add_compile_options(/TC) # compile all files as C
  add_compile_options(/permissive-)

  # files for cross os compilation
  include_directories(include/remote)
  include_directories(include/remote/win)

  # Warnings in release builds
  add_compile_options(-wd4068) # ignore unknown pragma warnings (gcc pragmas)
  add_compile_options(-wd4334) # 32-bit shift implicitly converted to 64 bits

  # Windows builds will only support remote unwind
  add_definitions(-DUNW_REMOTE_ONLY)

  # Disable security warnings
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)

  # Our posix abstraction layer will provide these headers
  set(HAVE_ELF_H 1)
  set(HAVE_ENDIAN_H 1)

  # MSVC compiler is currently missing C11 stdalign.h header
  # Fake it until support is added
  check_include_files(stdalign.h HAVE_STDALIGN_H)
  if (NOT HAVE_STDALIGN_H)
    configure_file(include/remote/win/fakestdalign.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/stdalign.h)
  endif (NOT HAVE_STDALIGN_H)

  # MSVC compiler is currently missing C11 stdatomic.h header
  # Fake it until support is added
  check_include_files(stdatomic.h HAVE_STDATOMIC_H)
  if (NOT HAVE_STDATOMIC_H)
    configure_file(include/remote/win/fakestdatomic.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/stdatomic.h)
  endif (NOT HAVE_STDATOMIC_H)

  # Check for threads.h (provides thread_local in C11)
  check_include_files(threads.h HAVE_THREADS_H)
  if (NOT HAVE_THREADS_H)
    # threads.h not available; check for thread_local as a builtin keyword (C23)
    check_c_source_compiles("void foo(void) { thread_local int a; }" HAVE_THREAD_LOCAL_KEYWORD)
    if (NOT HAVE_THREAD_LOCAL_KEYWORD)
      # Fall back to MSVC __declspec(thread)
      check_c_source_compiles("void foo(void) { __declspec(thread) int a; }" HAVE_DECLSPEC_THREAD)
      if (HAVE_DECLSPEC_THREAD)
        add_definitions(-Dthread_local=__declspec(thread))
      else ()
        add_definitions(-Dthread_local=)
      endif ()
    endif ()
  endif (NOT HAVE_THREADS_H)
else ()
  message(FATAL_ERROR "This CMake file is currently only designed for building on Visual Studio")
endif ()

add_definitions(-DHAVE_CONFIG_H)

configure_file(include/config.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)
configure_file(include/libunwind-common.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/libunwind-common.h)
configure_file(include/libunwind.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/libunwind.h)
configure_file(include/tdep/libunwind_i.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/tdep/libunwind_i.h)

add_subdirectory(src)

