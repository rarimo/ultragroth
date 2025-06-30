set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_RELEASE   "-O3")

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
    keccak256.cpp
    keccak256.h
    binfile_utils.hpp
    binfile_utils.cpp
    zkey_utils.hpp
    zkey_utils.cpp
    wtns_utils.hpp
    wtns_utils.cpp
    fileloader.cpp
    fileloader.hpp
    prover.cpp
    prover.h
    verifier.cpp
    verifier.h
    prover.cpp
    prover.h
    verifier.cpp
    verifier.h 
    ../depends/ffiasm/c/misc.cpp
    ../depends/ffiasm/c/naf.cpp
    ../depends/ffiasm/c/splitparstr.cpp
    ../depends/ffiasm/c/alt_bn128.cpp
)

include_directories(
    ../src
    ../build
    ../depends/ffiasm/c
    ../depends/json/single_include
)

add_library(ultragrothStatic STATIC ${LIB_SOURCES})
set_target_properties(ultragrothStatic PROPERTIES OUTPUT_NAME ultragroth)

add_library(ultragrothStaticFrFq STATIC ${LIB_SOURCES} ${FQ_SOURCES} ${FR_SOURCES})
set_target_properties(ultragrothStaticFrFq PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(ultragrothStaticFrFq PROPERTIES OUTPUT_NAME ultragroth-fr-fq)

add_executable(prover main_prover.cpp)
target_link_libraries(prover ultragrothStatic)

add_executable(verifier main_verifier.cpp)
target_link_libraries(verifier ultragrothStatic)

add_library(ultragroth SHARED ${LIB_SOURCES})

if(NOT USE_OPENMP AND NOT TARGET_PLATFORM MATCHES "android")
    target_link_libraries(prover pthread)
    target_link_libraries(verifier pthread)
endif()

if(USE_SODIUM)
    target_link_libraries(prover sodium)
endif()

add_executable(prover_ultra_groth main_prover_ultra_groth.cpp)
target_link_libraries(prover_ultra_groth ultragrothStatic)

add_executable(verifier_ultra_groth main_verifier_ultra_groth.cpp)
target_link_libraries(verifier_ultra_groth ultragrothStatic)

if(OpenMP_CXX_FOUND)

    if(TARGET_PLATFORM MATCHES "android")
        target_link_libraries(prover -static-openmp -fopenmp)
        target_link_libraries(verifier -static-openmp -fopenmp)
        target_link_libraries(ultragroth -static-openmp -fopenmp)

    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(prover OpenMP::OpenMP_CXX)
        target_link_libraries(verifier OpenMP::OpenMP_CXX)
    endif()

endif()


