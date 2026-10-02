cmake_minimum_required(VERSION 3.24)
project(PandoraLite LANGUAGES CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Configure a Windows x64 build.")
endif()
include(FetchContent)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.9b GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)
add_executable(pandora_lite WIN32 pandora_app.cpp
 ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
 ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
 ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
target_include_directories(pandora_lite PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_features(pandora_lite PRIVATE cxx_std_20)
target_compile_definitions(pandora_lite PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
if(MSVC)
 target_compile_options(pandora_lite PRIVATE /W4 /EHsc /utf-8)
 set_property(TARGET pandora_lite PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_link_libraries(pandora_lite PRIVATE d3d11 dxgi dwmapi user32 gdi32 winhttp bcrypt shell32)

# Portable state-transition tests use fake character memory; no Roblox needed.
add_executable(movement_tests tests_movement.cpp)
target_compile_features(movement_tests PRIVATE cxx_std_20)
if(MSVC)
 target_compile_options(movement_tests PRIVATE /W4 /EHsc /utf-8)
 set_property(TARGET movement_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
add_custom_command(TARGET movement_tests POST_BUILD COMMAND $<TARGET_FILE:movement_tests>
 COMMENT "Checking movement restoration, focus loss and session replacement" VERBATIM)

if(DEFINED ENV{GITHUB_RUN_NUMBER})
 target_compile_definitions(pandora_lite PRIVATE PANDORA_BUILD_NUMBER=$ENV{GITHUB_RUN_NUMBER})
endif()
