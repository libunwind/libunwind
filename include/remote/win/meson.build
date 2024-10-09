# Set up the POSIX abstraction layer
internal_inc += include_directories('.')

# MSVC may not have the C11 stdalign.h header. Fake it for now.
if not cc.check_header('stdalign.h')
    configure_file(
        output: 'stdalign.h',
        input: 'fakestdalign.h.in',
        configuration: cfg,
    )
endif

# MSVC may not have the C11 stdatomic.h header. Fake it for now.
if not cc.check_header('stdatomic.h')
    configure_file(
        output: 'stdatomic.h',
        input: 'fakestdatomic.h.in',
        configuration: cfg,
    )
endif

# MSVC may not have C11 _Thread_local. Fake it for now.
if not cc.compiles('void main() { _Thread_local int a; }')
    add_project_arguments('-D_Thread_local=', language: ['c', 'cpp'])
endif
