# Checks symbols.toml and symbols.lock for facts a review had to correct
# (hamweard-review of 17.11.2026): run by ctest as symbols_check.
file(READ "${SOURCE_DIR}/symbols.toml" toml)
file(READ "${SOURCE_DIR}/symbols.lock" lock)
set(failed FALSE)

function(fail what)
    message(SEND_ERROR "symbols_check: ${what}")
    set(failed TRUE PARENT_SCOPE)
endfunction()

# The note of a symbol: the first note line after its [symbol.NAME] header.
function(note_of name out)
    string(FIND "${toml}" "[symbol.${name}]" at)
    if(at EQUAL -1)
        set(${out} "" PARENT_SCOPE)
        return()
    endif()
    string(SUBSTRING "${toml}" ${at} -1 rest)
    string(REGEX MATCH "\nnote = \"[^\n]*\"" note "${rest}")
    set(${out} "${note}" PARENT_SCOPE)
endfunction()

# What the functions the stock Exit to Menu calls do, in the review's words.
foreach(pair "in_game_exit=leaves the session for a new single-player one"
             "campaign_exit_cleanup=saves an ironman campaign"
             "notifications_destroy=pending diplomatic icons"
             "reset_game=slot \\+0x140")
    string(REPLACE "=" ";" pair "${pair}")
    list(GET pair 0 name)
    list(GET pair 1 expected)
    note_of(${name} note)
    if(NOT note MATCHES "${expected}")
        fail("the note of ${name} does not say ${expected}: ${note}")
    endif()
endforeach()

# Every declared build has a structure layout (src/build_layout.hpp) and a passing check in the lock.
string(REGEX MATCH "\nbuilds = \\[[^]]*\\]" builds "${toml}")
string(REGEX MATCHALL "\"[^\"]+\"" builds "${builds}")
if(NOT builds)
    fail("no builds list in symbols.toml")
endif()
foreach(build IN LISTS builds)
    string(REPLACE "\"" "" build "${build}")
    if(NOT build MATCHES "-(steam|gog)-win64$")
        fail("${build}: no structure layout for it in src/build_layout.hpp")
    endif()
    string(REPLACE "." "\\." pattern "${build}")
    if(NOT lock MATCHES "build = \"${pattern}\"\r?\nexe_sha256 = \"[0-9A-F]+\"\r?\nresult = \"ok\"")
        fail("${build}: symbols.lock has no passing check")
    endif()
endforeach()

if(failed)
    message(FATAL_ERROR "symbols_check failed")
endif()
list(LENGTH builds count)
message(STATUS "symbols_check: notes and ${count} builds ok")
