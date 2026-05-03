##############################################################################
# CMakeLists.txt — AUTOSAR R24-11 Fee + MemAcc  Host Unit / Integration Tests
#
# Targets
#   fee_lib      – static library from src/Fee*.c
#   memacc_lib   – static library from src/MemAcc*.c
#   unity        – Unity test framework (object library)
#   stubs        – test stubs   (object library)
#   <test_*>     – individual test executables
#
# Usage:
#   mkdir build && cd build
#   cmake .. && make -j$(nproc)
#   ctest --output-on-failure
##############################################################################

cmake_minimum_required(VERSION 3.10)
project(Fee_MemAcc_Production C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Compiler flags — strict, plus unit-test macro
add_compile_options(-Wall -Wextra -Werror -pedantic -DFEE_UNIT_TEST)

# Common include paths
set(COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/test/stubs
    ${CMAKE_SOURCE_DIR}/test/unity
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_SOURCE_DIR}/test/helpers
)

include_directories(${COMMON_INCLUDES})

##############################################################################
# Libraries
##############################################################################

# --- Fee static library ---
file(GLOB FEE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/Fee.c
    ${CMAKE_SOURCE_DIR}/src/Fee_StateMachine.c
    ${CMAKE_SOURCE_DIR}/src/Fee_GarbageCollect.c
    ${CMAKE_SOURCE_DIR}/src/Fee_Sector.c
    ${CMAKE_SOURCE_DIR}/src/Fee_Crc.c
    ${CMAKE_SOURCE_DIR}/src/Fee_Safety.c
    ${CMAKE_SOURCE_DIR}/src/Fee_Cfg.c
    ${CMAKE_SOURCE_DIR}/src/Fee_PBcfg.c
)
add_library(fee_lib STATIC ${FEE_SOURCES})

# --- MemAcc static library ---
file(GLOB MEMACC_SOURCES
    ${CMAKE_SOURCE_DIR}/src/MemAcc.c
    ${CMAKE_SOURCE_DIR}/src/MemAcc_JobProcessing.c
    ${CMAKE_SOURCE_DIR}/src/MemAcc_AddressArea.c
    ${CMAKE_SOURCE_DIR}/src/MemAcc_Cfg.c
    ${CMAKE_SOURCE_DIR}/src/MemAcc_PBcfg.c
)
add_library(memacc_lib STATIC ${MEMACC_SOURCES})

# --- Unity framework ---
add_library(unity OBJECT ${CMAKE_SOURCE_DIR}/test/unity/unity.c)

# --- Test stubs ---
file(GLOB STUB_SOURCES ${CMAKE_SOURCE_DIR}/test/stubs/*.c)
add_library(stubs OBJECT ${STUB_SOURCES})

# --- Shared test helpers ---
add_library(test_helpers OBJECT ${CMAKE_SOURCE_DIR}/test/helpers/test_Fee_Helpers.c)

##############################################################################
# Helper: add a test executable
##############################################################################
function(add_fee_test TEST_NAME TEST_SOURCE TEST_GROUP)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME}
        fee_lib memacc_lib
        $<TARGET_OBJECTS:unity>
        $<TARGET_OBJECTS:stubs>
        $<TARGET_OBJECTS:test_helpers>
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES LABELS "${TEST_GROUP}")
endfunction()

##############################################################################
# Unit tests (existing)
##############################################################################

enable_testing()

# Discover all unit test source files
file(GLOB UNIT_TEST_SOURCES ${CMAKE_SOURCE_DIR}/test/unit/test_*.c)
foreach(TEST_SRC ${UNIT_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_fee_test(${TEST_NAME} ${TEST_SRC} "unit")
endforeach()

##############################################################################
# Integration tests
##############################################################################

file(GLOB INTEGRATION_TEST_SOURCES ${CMAKE_SOURCE_DIR}/test/integration/test_*.c)
foreach(TEST_SRC ${INTEGRATION_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_fee_test(${TEST_NAME} ${TEST_SRC} "integration")
endforeach()
