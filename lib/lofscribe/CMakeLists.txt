add_custom_target(lofscribe)

set(LOFSCRIBE_SOURCES lofscribe.c )
set(LOFSCRIBE_FLAGS ${SANITIZER_COMMON_FLAGS} )

include_directories(..)

add_compiler_rt_runtime(clang_rt.lofscribe 
        STATIC 
        ARCHS x86_64 
        SOURCES ${LOFSCRIBE_SOURCES}
        CFLAGS ${LOFSCRIBE_FLAGS}
		PARENT_TARGET lofscribe)
