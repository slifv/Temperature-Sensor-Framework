# ============================================================
#  Sensor Framework CMake Build Configuration
# ============================================================
#  Usage:
#    cmake .. -DSENSOR_PROFILE=STANDARD
#    cmake .. -DSENSOR_PROFILE=ADVANCED
#    cmake .. -DSENSOR_PROFILE=MINIMAL
#    cmake .. -DSENSOR_PROFILE=MINIMAL_8BIT
#    cmake .. -DSENSOR_PROFILE=CUSTOM
#    cmake .. -DBUILD_TESTS=ON
#    cmake .. -DTEST_MODE=WHITEBOX
#    cmake .. -DTEST_MODE=BLACKBOX
# ============================================================

cmake_minimum_required(VERSION 3.10)
project(SensorFramework VERSION 1.0.0 LANGUAGES CXX C)

# ─── C++ Standard ──────────────────────────────────────
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable RTTI and exceptions for embedded targets
option(SENSOR_NO_RTTI "Disable RTTI" ON)
option(SENSOR_NO_EXCEPTIONS "Disable C++ exceptions" ON)

if(SENSOR_NO_RTTI)
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
    endif()
endif()

if(SENSOR_NO_EXCEPTIONS)
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
    endif()
endif()

# ─── Sensor Profile ────────────────────────────────────
set(SENSOR_PROFILE "STANDARD" CACHE STRING "Sensor profile: MINIMAL|STANDARD|ADVANCED|MINIMAL_8BIT|CUSTOM")

if(SENSOR_PROFILE STREQUAL "MINIMAL")
    add_definitions(-DSENSOR_PROFILE_MINIMAL)
elseif(SENSOR_PROFILE STREQUAL "STANDARD")
    add_definitions(-DSENSOR_PROFILE_STANDARD)
elseif(SENSOR_PROFILE STREQUAL "ADVANCED")
    add_definitions(-DSENSOR_PROFILE_ADVANCED)
elseif(SENSOR_PROFILE STREQUAL "MINIMAL_8BIT")
    add_definitions(-DSENSOR_PROFILE_MINIMAL_8BIT)
elseif(SENSOR_PROFILE STREQUAL "CUSTOM")
    add_definitions(-DSENSOR_PROFILE_CUSTOM)
endif()

# ─── Feature Overrides ─────────────────────────────────
# Allow individual features to be overridden via cmake -D
function(define_feature name default_value)
    if(NOT DEFINED ${name})
        set(${name} ${default_value} CACHE BOOL "Enable ${name}")
    endif()
    if(${name})
        add_definitions(-D${name}=1)
    else()
        add_definitions(-D${name}=0)
    endif()
endfunction()

define_feature(SENSOR_FEATURE_FILTER_MEAN ON)
define_feature(SENSOR_FEATURE_FILTER_MEDIAN ON)
define_feature(SENSOR_FEATURE_FILTER_LOWPASS ON)
define_feature(SENSOR_FEATURE_FILTER_KALMAN ON)
define_feature(SENSOR_FEATURE_FILTER_SLIDING_WINDOW ON)
define_feature(SENSOR_FEATURE_FILTER_TRIMMED_MEAN ON)
define_feature(SENSOR_FEATURE_EVENTBUS ON)
define_feature(SENSOR_FEATURE_EXCEPTION ON)

# ─── Header-Only Library ───────────────────────────────
# The framework is header-only, but we create an interface library
# for IDE integration and dependency management
add_library(sensor_framework INTERFACE)
target_include_directories(sensor_framework INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(sensor_framework INTERFACE cxx_std_11)

# ─── Tests ─────────────────────────────────────────────
option(BUILD_TESTS "Build test suite" OFF)
option(TEST_MODE "Test mode: WHITEBOX|BLACKBOX" "WHITEBOX")

if(BUILD_TESTS)
    # Enable test framework
    add_definitions(-DSENSOR_FEATURE_TEST_FRAMEWORK=1)

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        # PC-side tests
        enable_testing()

        # Whitebox tests
        add_executable(test_filter
            tests/whitebox/test_filter.cpp)
        target_include_directories(test_filter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_filter PRIVATE sensor_framework)

        add_executable(test_event_bus
            tests/whitebox/test_event_bus.cpp)
        target_include_directories(test_event_bus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_event_bus PRIVATE sensor_framework)

        add_executable(test_sensor_manager
            tests/whitebox/test_sensor_manager.cpp)
        target_include_directories(test_sensor_manager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_sensor_manager PRIVATE sensor_framework)

        add_executable(test_exception
            tests/whitebox/test_exception.cpp)
        target_include_directories(test_exception PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_exception PRIVATE sensor_framework)

        add_executable(test_static_vector
            tests/whitebox/test_static_vector.cpp)
        target_include_directories(test_static_vector PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_static_vector PRIVATE sensor_framework)

        add_executable(test_sensor_base
            tests/whitebox/test_sensor_base.cpp)
        target_include_directories(test_sensor_base PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_sensor_base PRIVATE sensor_framework)

        add_executable(test_ring_buffer
            tests/whitebox/test_ring_buffer.cpp)
        target_include_directories(test_ring_buffer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_ring_buffer PRIVATE sensor_framework)

        # Blackbox tests
        add_executable(test_sensor_pipeline
            tests/blackbox/test_sensor_pipeline.cpp)
        target_include_directories(test_sensor_pipeline PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(test_sensor_pipeline PRIVATE sensor_framework)

        # Register tests with CTest
        add_test(NAME Filter            COMMAND test_filter)
        add_test(NAME EventBus          COMMAND test_event_bus)
        add_test(NAME SensorManager     COMMAND test_sensor_manager)
        add_test(NAME Exception         COMMAND test_exception)
        add_test(NAME StaticVector      COMMAND test_static_vector)
        add_test(NAME SensorBase        COMMAND test_sensor_base)
        add_test(NAME RingBuffer        COMMAND test_ring_buffer)
        add_test(NAME SensorPipeline    COMMAND test_sensor_pipeline)

        message(STATUS "Tests enabled: 8 test executables")
    endif()
endif()

# ─── Profile Info ───────────────────────────────────────
message(STATUS "Sensor Framework Configuration:")
message(STATUS "  Profile:       ${SENSOR_PROFILE}")
message(STATUS "  C++ Standard:  C++11")
message(STATUS "  Tests:         ${BUILD_TESTS}")
message(STATUS "  RTTI:          ${SENSOR_NO_RTTI}")
message(STATUS "  Exceptions:    ${SENSOR_NO_EXCEPTIONS}")
