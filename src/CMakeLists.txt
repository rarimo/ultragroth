set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native")
set(CMAKE_C_FLAGS_RELEASE   "-O3 -march=native -mtune=native")

link_libraries(${GMP_LIB})

add_definitions(${GMP_DEFINIONS})

if(USE_ASM)
    if(ARCH MATCHES "arm64")
            add_definitions(-DUSE_ASM -DARCH_ARM64)
    elseif(ARCH MATCHES "x86_64")
            add_definitions(-DUSE_ASM -DARCH_X86_64)
    endif()
endif()

if(DEFINED BITS_PER_CHUNK)
    add_definitions(-DMSM_BITS_PER_CHUNK=${BITS_PER_CHUNK})
endif()

if(USE_ASM AND ARCH MATCHES "x86_64")

    if (CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin" AND NOT TARGET_PLATFORM MATCHES "^android(_x86_64)?")
        set(NASM_FLAGS -fmacho64 --prefix _)
    else()
        set(NASM_FLAGS -felf64 -DPIC)
    endif()

    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/build/fq_asm.o
        COMMAND nasm ${NASM_FLAGS} fq.asm -o fq_asm.o
        DEPENDS ${CMAKE_SOURCE_DIR}/build/fq.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build
    )

    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/build/fr_asm.o
        COMMAND nasm ${NASM_FLAGS} fr.asm -o fr_asm.o
        DEPENDS ${CMAKE_SOURCE_DIR}/build/fr.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build
    )
endif()

set(
    FR_SOURCES
    ../build/fr.hpp
    ../build/fr.cpp
)

if(USE_ASM)
    if(ARCH MATCHES "arm64")
        set(FR_SOURCES ${FR_SOURCES} ../build/fr_raw_arm64.s ../build/fr_raw_generic.cpp ../build/fr_generic.cpp)
    elseif(ARCH MATCHES "x86_64")
        set(FR_SOURCES ${FR_SOURCES} ../build/fr_asm.o)
    endif()
else()
    set(FR_SOURCES ${FR_SOURCES} ../build/fr_generic.cpp ../build/fr_raw_generic.cpp)
endif()

add_library(fr STATIC ${FR_SOURCES})
set_target_properties(fr PROPERTIES POSITION_INDEPENDENT_CODE ON)

link_libraries(fr)

set(
    FQ_SOURCES
    ../build/fq.hpp
    ../build/fq.cpp
)

if(USE_ASM)
    if(ARCH MATCHES "arm64")
        set(FQ_SOURCES ${FQ_SOURCES} ../build/fq_raw_arm64.s ../build/fq_raw_generic.cpp ../build/fq_generic.cpp)
    elseif(ARCH MATCHES "x86_64")
        set(FQ_SOURCES ${FQ_SOURCES} ../build/fq_asm.o)
    endif()
else()
    set(FQ_SOURCES ${FQ_SOURCES} ../build/fq_raw_generic.cpp ../build/fq_generic.cpp)
endif()

add_library(fq STATIC ${FQ_SOURCES})
set_target_properties(fq PROPERTIES POSITION_INDEPENDENT_CODE ON)

link_libraries(fq)


if(OpenMP_CXX_FOUND)
    add_definitions(-DUSE_OPENMP)
    add_compile_options(${OpenMP_CXX_FLAGS})
endif()

set(
    LIB_SOURCES
    binfile_utils.hpp
    binfile_utils.cpp
    zkey_utils.hpp
    zkey_utils.cpp
    fileloader.cpp
    fileloader.hpp
    rounds_extern.h
    prover_ultra_groth.cpp
    prover_ultra_groth.hpp
    verifier_ultra_groth.cpp
    verifier_ultra_groth.hpp
    ../depends/ffiasm/c/misc.cpp
    ../depends/ffiasm/c/naf.cpp
    ../depends/ffiasm/c/splitparstr.cpp
    ../depends/ffiasm/c/alt_bn128.cpp
)

if(USE_LOGGER)
    set(LIB_SOURCES ${LIB_SOURCES} logger.cpp)
    add_definitions(-DUSE_LOGGER)
endif()

include_directories(
    ../src
    ../build
    ../depends/ffiasm/c
    ../depends/json/single_include
)

add_library(rapidsnarkStatic STATIC ${LIB_SOURCES})
set_target_properties(rapidsnarkStatic PROPERTIES OUTPUT_NAME rapidsnark)

add_library(rapidsnarkStaticFrFq STATIC ${LIB_SOURCES} ${FQ_SOURCES} ${FR_SOURCES})
set_target_properties(rapidsnarkStaticFrFq PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(rapidsnarkStaticFrFq PROPERTIES OUTPUT_NAME rapidsnark-fr-fq)


find_package(OpenSSL REQUIRED)
add_executable(prover_ultra_groth main_prover_ultra_groth.cpp)
target_link_libraries(prover_ultra_groth rapidsnarkStatic OpenSSL::Crypto ${BIONETTA_PROVER_LIB})


find_package(OpenSSL REQUIRED)
add_executable(verifier_ultra_groth main_verifier_ultra_groth.cpp)
target_link_libraries(verifier_ultra_groth rapidsnarkStatic OpenSSL::Crypto)

add_library(rapidsnark SHARED ${LIB_SOURCES})
