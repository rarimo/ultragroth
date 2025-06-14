cmake_minimum_required(VERSION 3.10)

include(cmake/platform.cmake)

set(USE_ASM    ON CACHE BOOL "Use asm implementation for Fr and Fq")
set(USE_OPENMP ON CACHE BOOL "Use OpenMP")

project(rapidsnark LANGUAGES CXX C ASM)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# TODO Maybe pass path as argument to cmake configuration
# Linux
if (DEFINED CMAKE_LIB_PATH)
    message("Found path to lib: ${CMAKE_LIB_PATH}.")
    set(BIONETTA_PROVER_LIB ${CMAKE_LIB_PATH})

elseif(UNIX AND NOT APPLE)
    set(BIONETTA_PROVER_LIB ${CMAKE_SOURCE_DIR}/libtest_mnist.a)

# Mac OS
elseif(APPLE)
    set(BIONETTA_PROVER_LIB ${CMAKE_SOURCE_DIR}/libtest_mnist.a)

# Windows
else()
    set(BIONETTA_PROVER_LIB ${CMAKE_SOURCE_DIR}/libtest_mnist.a)
endif()


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
    TARGETS rapidsnark rapidsnarkStatic rapidsnarkStaticFrFq fr fq
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX}/app
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(
    FILES "${GMP_LIB_DIR}/${GMP_LIB_FILE}"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

enable_testing()
