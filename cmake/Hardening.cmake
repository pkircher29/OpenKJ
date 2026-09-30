# Apply before adding the application and bundled libraries.
include(CheckCXXCompilerFlag)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif ()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (MSVC)
    add_compile_options(/sdl /guard:cf)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " /guard:cf")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fstack-protector-strong -Wformat -Wformat-security)
    foreach(flag -fstack-clash-protection -fcf-protection=full)
        string(MAKE_C_IDENTIFIER "OKJ_HAS_${flag}" flag_var)
        check_cxx_compiler_flag("${flag}" ${flag_var})
        if (${flag_var})
            add_compile_options("${flag}")
        endif ()
    endforeach()
    # Level 2 also supports older libc/toolchain combinations. It requires
    # optimisation, so do not define it for Debug or an unspecified config.
    add_compile_options("$<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:-D_FORTIFY_SOURCE=2>")
    if (CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD")
        string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,-z,relro,-z,now,-z,noexecstack")
    endif ()
endif ()
