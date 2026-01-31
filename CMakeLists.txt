# Symbolic link for debug version:

# mklink /D "C:\Program Files\Common Files\VST3\CounterTune_v2.vst3" "C:\Users\savas\Documents\JUCE Projects\CounterTune_v2\cmake_build\CounterTune_artefacts\Debug\VST3\CounterTune_v2.vst3"

# visual studio project preferences:

# command: C:\Program Files (x86)\Image-Line\FL Studio 12\FL64.exe


cmake_minimum_required(VERSION 3.15)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures for macOS")
endif()

project(CounterTune VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# Build configuration options
option(DEMO_BUILD "Build demo version" OFF)

# JUCE path
if(APPLE)
    set(JUCE_PATH "/Users/admin/Documents/repos/JUCE")
elseif(NOT APPLE)
    set(JUCE_PATH "C:/Program Files/JUCE")
endif()

# Include JUCE
add_subdirectory(${JUCE_PATH} JUCE)

# Conditional formats
if(APPLE)
    set(PLUGIN_FORMATS AU VST3)
elseif(NOT APPLE)
    set(PLUGIN_FORMATS VST3)
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    add_compile_options(
        $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/MT>
    )
endif()

# Conditional product name
if(DEMO_BUILD)
    set(PLUGIN_PRODUCT_NAME "CounterTune_v2_Demo")
else()
    set(PLUGIN_PRODUCT_NAME "CounterTune_v2")
endif()

# Define user library path for plugin installation (macOS-specific)
if(APPLE)
    set(USER_LIBRARY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins")
endif()

# Plugin setup arguments (common)
set(PLUGIN_ARGS
    PLUGIN_MANUFACTURER_CODE "Manu"
    PLUGIN_CODE "CtnE"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "${PLUGIN_PRODUCT_NAME}"
    COMPANY_NAME "54v450l4r"
    BUNDLE_ID "com.54v450l4r.countertune"
    PLUGIN_IS_A_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    IS_MIDI_EFFECT FALSE
    IS_SYNTH FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    MIDI_INPUT_NAME "MIDI Input"
    MIDI_OUTPUT_NAME "MIDI Output"
)

# Conditional plugin setup arguments
if(APPLE)
    list(APPEND PLUGIN_ARGS
        COPY_PLUGIN_AFTER_BUILD TRUE
        AU_COPY_DIR "${USER_LIBRARY_DIR}/Components"
        VST3_COPY_DIR "${USER_LIBRARY_DIR}/VST3"
        VST3_COPY_DIR_DISABLE_AUTO_COPY TRUE
    )
else()
    list(APPEND PLUGIN_ARGS
        COPY_PLUGIN_AFTER_BUILD FALSE
    )
endif()

# Plugin setup
juce_add_plugin(CounterTune ${PLUGIN_ARGS})

target_compile_definitions(CounterTune
    PRIVATE
        JUCE_DISABLE_VST2_BUILDING=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
        $<$<BOOL:${DEMO_BUILD}>:DEMO_BUILD>
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(CounterTune PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/dywapitchtrack/
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/dywapitchtrack/src
)

# Source files

target_sources(CounterTune PRIVATE
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Dependencies/dywapitchtrack/src/dywapitchtrack.c
)

set_source_files_properties(Dependencies/dywapitchtrack/src/dywapitchtrack.c PROPERTIES LANGUAGE C)

# Binary data
juce_add_binary_data(BinaryResources SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Resources/placeholder.bmp
)

# Link everything
target_link_libraries(CounterTune PRIVATE
    BinaryResources
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_osc
)

juce_generate_juce_header(CounterTune)