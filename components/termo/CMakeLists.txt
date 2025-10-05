add_subdirectory(common)
add_subdirectory(packet_task)
add_subdirectory(display_task)
add_subdirectory(control_task)
add_subdirectory(system_task)

add_library(termo STATIC)

target_sources(termo PRIVATE 
    termo.c
)

target_include_directories(termo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(termo PUBLIC
    common
    control_task
    packet_task
    system_task
    display_task
)

target_compile_options(termo PUBLIC
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

