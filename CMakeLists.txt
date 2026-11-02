cmake_minimum_required(VERSION 3.25)
project(hamweard LANGUAGES CXX)

include(FetchContent)

# The Anvil SDK: the AnvilPlugin CMake package (anvil_add_plugin) and anvil.exe,
# from the SDK archive of an Anvil release. URL_HASH is the archive's line in
# the release's SHA256SUMS.txt.
FetchContent_Declare(anvil_sdk
    URL https://github.com/happy-dwarf-team/anvil/releases/download/v1.5.2/anvil-sdk-windows-x64.zip
    URL_HASH SHA256=a229d0a0edc57aaa5529784447be8750d518315cf93c98b8de23bdc97e4a6762)
FetchContent_MakeAvailable(anvil_sdk)

# Reads plugin.toml, runs `anvil gen` on symbols.toml, builds the DLL and lays
# out build/plugin/<id>/. Adds hamweard-check and, with -DEU4_DIR=<game
# folder>, hamweard-deploy.
anvil_add_plugin(hamweard SOURCES src/plugin.cpp)

# The world-reset gate on its own (no game needed).
enable_testing()
add_executable(menu_transition_tests tests/menu_transition_tests.cpp)
target_include_directories(menu_transition_tests PRIVATE src)
target_compile_features(menu_transition_tests PRIVATE cxx_std_20)
add_test(NAME menu_transition_tests COMMAND menu_transition_tests)
