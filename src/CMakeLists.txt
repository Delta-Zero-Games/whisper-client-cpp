# Enable Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Collect source files
file(GLOB_RECURSE SOURCES
    "*.cpp"
    "audio/*.cpp"
    "config/*.cpp"
    "input/*.cpp"
    "network/*.cpp"
    "ui/*.cpp"
)

# Collect header files
file(GLOB_RECURSE HEADERS
    "*.hpp"
    "audio/*.hpp"
    "config/*.hpp"
    "input/*.hpp"
    "network/*.hpp"
    "ui/*.hpp"
)

# Create executable
add_executable(whisper-client
    ${SOURCES}
    ${HEADERS}
)

# Include directories
target_include_directories(whisper-client
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(whisper-client
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::WebSockets
        whisper
        rtaudio
        nlohmann_json::nlohmann_json
)

# Compiler options
if(MSVC)
    target_compile_options(whisper-client PRIVATE /W4)
else()
    target_compile_options(whisper-client PRIVATE -Wall -Wextra -Wpedantic)
endif()