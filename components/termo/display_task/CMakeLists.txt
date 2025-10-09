add_subdirectory(display_utility)

add_library(display_task STATIC)

target_sources(display_task PRIVATE 
    display_manager.c
    display_task.c
)

target_link_libraries(display_task PUBLIC
    common
    stm32cubemx
    sh1107
    display_utility
)

target_include_directories(display_task PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(display_task PUBLIC
    -std=c23
    -Wall
    -Wextra
    -Wconversion
    -Wshadow
    -Wpedantic
    -Wnarrowing
    -Waddress
    -pedantic
    -Wdeprecated
    -Wsign-conversion
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wnull-dereference
    -Wdouble-promotion
    -Wimplicit-fallthrough
    -Wcast-align
    -Wformat=2
    -Wformat-security
    -Wmissing-prototypes
    -Wmissing-declarations
    -Wstrict-prototypes
    -Wold-style-definition
    -Wundef
    -Wvla
    -Wpointer-arith
    -Wstrict-aliasing=2
)
