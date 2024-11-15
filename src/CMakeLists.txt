# Add executable
add_executable(whisper-client
    main.cpp
)

# Include directories
target_include_directories(whisper-client
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Compiler options
if(MSVC)
    target_compile_options(whisper-client PRIVATE /W4)
else()
    target_compile_options(whisper-client PRIVATE -Wall -Wextra -Wpedantic)
endif()