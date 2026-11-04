cmake_minimum_required(VERSION 3.25)
project(hamweard LANGUAGES CXX)

include(FetchContent)

# The Anvil SDK: the AnvilPlugin CMake package (anvil_add_plugin) and anvil.exe,
# from the SDK archive of an Anvil release. URL_HASH is the archive's line in
# the release's SHA256SUMS.txt.
FetchContent_Declare(anvil_sdk
    URL https://github.com/happy-dwarf-team/anvil/releases/download/v1.9.0/anvil-sdk-windows-x64.zip
    URL_HASH SHA256=307efed144e4ff6e543ce28d435ab1820803cffb98b8efd5b37d3271944268ab)
FetchContent_MakeAvailable(anvil_sdk)

# Reads plugin.toml, runs `anvil gen` on symbols.toml, builds the DLL and lays
# out build/plugin/<id>/. Adds hamweard-check and, with -DEU4_DIR=<game
# folder>, hamweard-deploy.
anvil_add_plugin(hamweard SOURCES src/plugin.cpp)

# The licence asks to go with every copy: the release zip and the Workshop mod are
# this folder.
add_custom_command(TARGET hamweard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "$<TARGET_FILE_DIR:hamweard>/LICENSE")

# The world-reset gate on its own (no game needed).
enable_testing()
add_executable(menu_transition_tests tests/menu_transition_tests.cpp)
target_include_directories(menu_transition_tests PRIVATE src)
target_compile_features(menu_transition_tests PRIVATE cxx_std_20)
add_test(NAME menu_transition_tests COMMAND menu_transition_tests)
