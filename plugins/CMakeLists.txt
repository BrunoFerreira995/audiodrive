# JUCE is optional and separately licensed (AGPLv3/commercial); see plugins/README.md.
if(NOT APPLE)
    message(FATAL_ERROR "The AudioUnit plugin target requires macOS")
endif()
set(AUDIO32_JUCE_SOURCE_DIR "" CACHE PATH "Existing JUCE 8.0.12 source checkout")
if(AUDIO32_JUCE_SOURCE_DIR)
    add_subdirectory(${AUDIO32_JUCE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.12.tar.gz
        URL_HASH SHA256=dec1a8baee5aaec4502b717a421b850e257ab7e9c2efd03766185ec1957cf102)
    FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(Audio32Plugin
    COMPANY_NAME Audio32
    BUNDLE_ID com.audio32.effects
    PLUGIN_MANUFACTURER_CODE A032
    PLUGIN_CODE Au32
    FORMATS AU
    PRODUCT_NAME "Audio32 Effects"
    COPY_PLUGIN_AFTER_BUILD FALSE
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE)
target_sources(Audio32Plugin PRIVATE Audio32Processor.cpp)
target_compile_definitions(Audio32Plugin PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(Audio32Plugin PRIVATE audio32 juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
add_executable(audio32_au_host_tests ${PROJECT_SOURCE_DIR}/tests/au_host_tests.cpp)
target_link_libraries(audio32_au_host_tests PRIVATE audio32)
add_dependencies(audio32_au_host_tests Audio32Plugin_AU)
add_test(NAME audio32_au_host_tests COMMAND audio32_au_host_tests $<TARGET_FILE:Audio32Plugin_AU>)
