cmake_minimum_required(VERSION 3.25)

project(Audio32
    VERSION 0.1.0
    DESCRIPTION "High-performance 32-bit audio driver for macOS"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(AUDIO32_SANITIZER "" CACHE STRING "Sanitizer: address-undefined or thread")
set_property(CACHE AUDIO32_SANITIZER PROPERTY STRINGS "" address-undefined thread)
if(AUDIO32_SANITIZER)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(FATAL_ERROR "Sanitizers require Clang or GCC")
    endif()
    if(AUDIO32_SANITIZER STREQUAL "address-undefined")
        set(audio32_sanitizer_flags -fsanitize=address,undefined)
    elseif(AUDIO32_SANITIZER STREQUAL "thread")
        set(audio32_sanitizer_flags -fsanitize=thread)
    else()
        message(FATAL_ERROR "Unsupported AUDIO32_SANITIZER")
    endif()
    add_compile_options(${audio32_sanitizer_flags} -fno-omit-frame-pointer)
    add_link_options(${audio32_sanitizer_flags})
endif()

option(AUDIO32_BUILD_BENCHMARKS "Build repeatable DSP latency benchmark" OFF)

add_library(audio32 STATIC
    src/audio_io.cpp
    src/audio_unit.cpp
    src/audio_driver.cpp
    src/avfoundation.cpp
    src/buffer.cpp
    src/coreaudio.cpp
    src/effect_chain.cpp
    src/native_audio.cpp
    src/input_capture.cpp
    src/dsp.cpp
    src/metal_visualization.cpp
    src/midi.cpp
    src/mixer.cpp
    src/platform.cpp
    src/simd.cpp
    src/wireless_audio.cpp)

add_library(audio32_miniaudio STATIC src/miniaudio.cpp)
target_include_directories(audio32_miniaudio PUBLIC ${PROJECT_SOURCE_DIR}/third_party/miniaudio)
set_target_properties(audio32_miniaudio audio32 PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(audio32 PRIVATE audio32_miniaudio ${CMAKE_DL_LIBS})

target_include_directories(audio32
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include)

if(APPLE)
    target_link_libraries(audio32
        PUBLIC
            "-framework AudioToolbox"
            "-framework AVFoundation"
            "-framework CoreAudio"
            "-framework CoreFoundation"
            "-framework Metal")
endif()

target_compile_features(audio32 PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(audio32 PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(audio32_example examples/basic_playback.cpp)
target_link_libraries(audio32_example PRIVATE audio32)

enable_testing()

add_executable(audio32_tests tests/audio32_tests.cpp)
target_link_libraries(audio32_tests PRIVATE audio32)

add_test(NAME audio32_tests COMMAND audio32_tests)

add_executable(audio32_dsp_tests tests/dsp_tests.cpp)
target_link_libraries(audio32_dsp_tests PRIVATE audio32)
add_test(NAME audio32_dsp_tests COMMAND audio32_dsp_tests)

find_package(Threads REQUIRED)
target_link_libraries(audio32_miniaudio PUBLIC Threads::Threads)
add_executable(audio32_phase6_tests tests/phase6_tests.cpp)
target_link_libraries(audio32_phase6_tests PRIVATE audio32)
add_test(NAME audio32_phase6_tests COMMAND audio32_phase6_tests ${PROJECT_SOURCE_DIR}/tests/fixtures/tone.mp3 ${PROJECT_SOURCE_DIR}/tests/fixtures/tone.flac)

add_executable(audio32_stress_tests tests/stress_tests.cpp)
target_link_libraries(audio32_stress_tests PRIVATE audio32 Threads::Threads)
add_test(NAME audio32_stress_tests COMMAND audio32_stress_tests)
set_tests_properties(audio32_stress_tests PROPERTIES LABELS "stress" TIMEOUT 120)

# Keep existing assertion-based tests active in Release builds too.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(audio32_tests PRIVATE -UNDEBUG)
elseif(MSVC)
    target_compile_options(audio32_tests PRIVATE /UNDEBUG)
endif()

if(AUDIO32_BUILD_BENCHMARKS)
    add_executable(audio32_latency_benchmark benchmarks/dsp_latency.cpp)
    target_link_libraries(audio32_latency_benchmark PRIVATE audio32)
endif()

install(TARGETS audio32 audio32_miniaudio
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(DIRECTORY include/
    DESTINATION include/audio32
    FILES_MATCHING PATTERN "*.hpp")

option(AUDIO32_BUILD_AU_PLUGIN "Build the optional JUCE AudioUnit effect plugin" OFF)
if(AUDIO32_BUILD_AU_PLUGIN)
    add_subdirectory(plugins)
endif()

option(AUDIO32_BUILD_HARDWARE_TOOLS "Build device diagnostics and physical latency tool" OFF)
if(AUDIO32_BUILD_HARDWARE_TOOLS)
    add_executable(audio32_hardware_benchmark tools/hardware_benchmark.cpp)
    target_link_libraries(audio32_hardware_benchmark PRIVATE audio32 Threads::Threads)
    add_executable(audio32_render_effects tools/render_effects.cpp)
    target_link_libraries(audio32_render_effects PRIVATE audio32)
endif()
