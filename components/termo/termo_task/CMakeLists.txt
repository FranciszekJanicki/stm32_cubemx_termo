add_library(termo_task STATIC)

target_sources(termo_task PRIVATE 
    termo_manager.c
    termo_task.c
)

target_include_directories(termo_task PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(termo_task PUBLIC
    common
    mcp9808
    pid_regulator
    stm32cubemx
)

target_compile_options(termo_task PUBLIC
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
