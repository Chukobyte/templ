cmake_minimum_required(VERSION 3.27.0)
project(templ C)

set(CMAKE_C_STANDARD 11)

# In the new_project superproject, seika is already defined by the root
# add_subdirectory(seika), which also defines the cjson target.  Standalone, fetch
# seika from GitHub — it brings cjson along.  To build against a local seika
# checkout instead: -DFETCHCONTENT_SOURCE_DIR_SEIKA=<path-to-seika>
if(NOT TARGET seika)
    # templ (a static site generator) never needs seika's graphics/audio/input/ECS
    # subsystems -- pre-seed the lean flags (mirrors koa/CMakeLists.txt's standalone
    # fallback and CI's -DSEIKA_*=OFF) so a plain standalone build doesn't silently
    # pull in freetype/glad/stb_image/miniaudio. Only runs when templ fetches its own
    # private seika; skipped in the monorepo where test_game needs the full-featured
    # shared seika target.
    set(SEIKA_RENDERING    OFF CACHE BOOL "" FORCE)
    set(SEIKA_AUDIO        OFF CACHE BOOL "" FORCE)
    set(SEIKA_ECS          OFF CACHE BOOL "" FORCE)
    set(SEIKA_INPUT        OFF CACHE BOOL "" FORCE)
    set(SEIKA_THREAD       OFF CACHE BOOL "" FORCE)
    set(SEIKA_HTTP_SERVER  OFF CACHE BOOL "" FORCE)
    set(SEIKA_MAIN         OFF CACHE BOOL "" FORCE)
    set(SEIKA_UNIT_TESTS   OFF CACHE BOOL "" FORCE)
    set(SEIKA_EXAMPLES     OFF CACHE BOOL "" FORCE)

    include(FetchContent)
    FetchContent_Declare(seika
        GIT_REPOSITORY https://github.com/Chukobyte/seika.git
        GIT_TAG v0.3.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(seika)
endif()

if (NOT DEFINED TEMPL_UNIT_TESTS)
    option(TEMPL_UNIT_TESTS "Will enable templ unit tests" ON)
endif ()

# Unity (unit testing) — reuse the one seika may have already fetched
if (TEMPL_UNIT_TESTS)
    # Always add these so templ test binaries see the same UNITY_INT width
    # as the compiled Unity library (which seika builds with UNITY_SUPPORT_64).
    add_definitions(-DUNITY_INCLUDE_DOUBLE)
    add_definitions(-DUNITY_SUPPORT_64)
    if (NOT TARGET unity)
        include(FetchContent)
        FetchContent_Declare(
            unity_content
            GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
            GIT_TAG v2.6.0
        )
        FetchContent_MakeAvailable(unity_content)
    endif ()
endif ()

# Core library (everything except main.c) — linked by both the CLI and tests
set(TEMPL_CORE_SRC
    src/platform.c
    src/config.c
    src/frontmatter.c
    src/markdown.c
    src/template.c
    src/engine.c
    src/theme.c
    src/themes/theme_default.c
    src/themes/theme_modern.c
)
add_library(templ_lib STATIC ${TEMPL_CORE_SRC})
target_include_directories(templ_lib PUBLIC src)
target_link_libraries(templ_lib PUBLIC seika cjson)

# CLI executable
add_executable(templ src/main.c)
target_link_libraries(templ PRIVATE templ_lib)

# Compiler warnings
set(TEMPL_FLAGS "")
if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    list(APPEND TEMPL_FLAGS /W3 /Zc:preprocessor)
    # See seika/CMakeLists.txt for why: MSVC's C4996 "unsafe" CRT warnings push
    # Microsoft-only _s replacements not available on Linux/macOS.
    target_compile_definitions(templ_lib PUBLIC _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else ()
    list(APPEND TEMPL_FLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
endif ()
target_compile_options(templ_lib PUBLIC ${TEMPL_FLAGS})

if (NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_compile_definitions(templ_lib PUBLIC _DEFAULT_SOURCE)
endif ()

# Unit tests — one executable per module
if (TEMPL_UNIT_TESTS)

enable_testing()

set(TEMPL_TEST_MODULES
    frontmatter
    markdown
    template
    platform
)

foreach(module ${TEMPL_TEST_MODULES})
    set(test_name templ_${module}_unit_tests)
    add_executable(${test_name} test/${module}_unit_test.c)
    target_link_libraries(${test_name} PRIVATE templ_lib unity)
    add_test(NAME ${module}_unit_tests COMMAND ${test_name})
endforeach()

endif () # TEMPL_UNIT_TESTS
