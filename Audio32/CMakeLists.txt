cmake_minimum_required(VERSION 3.25)

project(Audio32
    VERSION 0.1.0
    DESCRIPTION "High-performance 32-bit audio driver for macOS"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(audio32 STATIC
    src/audio_io.cpp
    src/audio_unit.cpp
    src/audio_driver.cpp
    src/avfoundation.cpp
    src/buffer.cpp
    src/coreaudio.cpp
    src/dsp.cpp
    src/midi.cpp
    src/mixer.cpp)

target_include_directories(audio32
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include)

if(APPLE)
    target_link_libraries(audio32
        PUBLIC
            "-framework AudioToolbox"
            "-framework AVFoundation"
            "-framework CoreAudio"
            "-framework CoreFoundation")
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

install(TARGETS audio32
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(DIRECTORY include/
    DESTINATION include/audio32
    FILES_MATCHING PATTERN "*.hpp")
