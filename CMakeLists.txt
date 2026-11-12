cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
endif()

project(Sadhana VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

option(SADHANA_BUILD_PLUGIN "Build the plug-in" ON)
option(SADHANA_BUILD_TEST "Build the offline render test" OFF)

if(SADHANA_BUILD_PLUGIN)
    set(SADHANA_FORMATS VST3)
    if(APPLE)
        list(APPEND SADHANA_FORMATS AU)
    endif()

    juce_add_plugin(Sadhana
        COMPANY_NAME "Sam Green"
        BUNDLE_ID "au.com.samgreengardencare.sadhana"
        PLUGIN_MANUFACTURER_CODE Sgrn
        PLUGIN_CODE Sdhn
        FORMATS ${SADHANA_FORMATS}
        PRODUCT_NAME "Sadhana"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        AU_MAIN_TYPE kAudioUnitType_Effect
        VST3_CATEGORIES Fx
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(Sadhana PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(Sadhana PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
    target_link_libraries(Sadhana
        PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
endif()

if(SADHANA_BUILD_TEST)
    juce_add_console_app(RenderTest PRODUCT_NAME "RenderTest")
    target_sources(RenderTest PRIVATE Test/RenderTest.cpp)
    target_compile_definitions(RenderTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(RenderTest PRIVATE juce::juce_audio_formats juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
endif()

option(SADHANA_BUILD_UISNAP "Build the UI snapshot tool" OFF)
if(SADHANA_BUILD_UISNAP)
    juce_add_gui_app(UISnapshot PRODUCT_NAME "UISnapshot")
    target_sources(UISnapshot PRIVATE Test/UISnapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(UISnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Sadhana")
    target_link_libraries(UISnapshot PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
endif()
