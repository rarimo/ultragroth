cmake_minimum_required(VERSION 3.10)

include(cmake/platform.cmake)

set(USE_ASM    ON CACHE BOOL "Use asm implementation for Fr and Fq")
set(USE_OPENMP ON CACHE BOOL "Use OpenMP")

project(ultragroth LANGUAGES CXX C ASM)

if(EXISTS "/proc/cpuinfo")
    file(READ "/proc/cpuinfo" CPUINFO)
    string(TOLOWER "${CPUINFO}" CPUINFO_LC)
    if(NOT CPUINFO_LC MATCHES "avx2")
        message(WARNING "CPU does not support AVX2 - disabling ASM optimizations.")
        set(USE_ASM OFF CACHE BOOL "Disable ASM optimizations" FORCE)
    endif()
    if(NOT CPUINFO_LC MATCHES "fma")
        message(WARNING "CPU does not support FMA - disabling ASM optimizations.")
        set(USE_ASM OFF CACHE BOOL "Disable ASM optimizations" FORCE)
    endif()
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("BITS_PER_CHUNK=" ${BITS_PER_CHUNK})
message("USE_ASM=" ${USE_ASM})
message("USE_OPENMP=" ${USE_OPENMP})
message("CMAKE_CROSSCOMPILING=" ${CMAKE_CROSSCOMPILING})

message("GMP_PREFIX=" ${GMP_PREFIX})
message("GMP_INCLUDE_DIR=" ${GMP_INCLUDE_DIR})
message("GMP_LIB_DIR=" ${GMP_LIB_DIR})

if (NOT EXISTS ${GMP_INCLUDE_FILE_FULLPATH})
    message("WARNING: ${GMP_INCLUDE_FILE_FULLPATH} is not found and so system ${GMP_INCLUDE_FILE} is used.")
endif()

if (NOT EXISTS ${GMP_LIB_FILE_FULLPATH})
    message("WARNING: ${GMP_LIB_FILE_FULLPATH} is not found and so system ${GMP_LIB_FILE} is used.")
    set(GMP_LIB gmp)
endif()


include_directories(BEFORE ${GMP_INCLUDE_DIR})

if(USE_OPENMP)
    find_package(OpenMP)

    if(OpenMP_CXX_FOUND)
        if(TARGET_PLATFORM MATCHES "android")
            message("OpenMP is used")

        elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
            message("OpenMP is used")

        else()
            set(OpenMP_CXX_FOUND FALSE)
            message("OpenMP is not used")

        endif()
    endif()
endif()


add_subdirectory(src)

install(
    TARGETS prover verifier ultragroth ultragrothStatic ultragrothStaticFrFq fr fq
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX}/app
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

# Check built-in GMP lib
if (EXISTS "/etc/os-release")
    file(READ "/etc/os-release" OS_RELEASE_CONTENT)
endif()

if (OS_RELEASE_CONTENT MATCHES "Arch Linux")
    message(STATUS "Detected Arch Linux - using system GMP.")
else()
    install(
        FILES "${GMP_LIB_DIR}/${GMP_LIB_FILE}"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    )
endif()

install(
    FILES src/prover.h src/verifier.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

enable_testing()
