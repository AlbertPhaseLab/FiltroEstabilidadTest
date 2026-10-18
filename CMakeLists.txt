cmake_minimum_required(VERSION 3.22)
project(FiltroEstabilidadTest VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically download and configure JUCE using FetchContent
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.5
)
FetchContent_MakeAvailable(JUCE)

# Create the console application
juce_add_console_app(FiltroEstabilidadTest
    PRODUCT_NAME "FiltroEstabilidadTest"
)

# Add the main source file
target_sources(FiltroEstabilidadTest PRIVATE Main.cpp)

juce_generate_juce_header(FiltroEstabilidadTest)

# Link the necessary JUCE modules
target_link_libraries(FiltroEstabilidadTest PRIVATE
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_dsp
)

