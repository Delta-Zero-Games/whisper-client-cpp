cmake_minimum_required(VERSION 3.15)

# Set CMake policies
cmake_policy(SET CMP0135 NEW)  # Set the DOWNLOAD_EXTRACT_TIMESTAMP policy to NEW

project(WhisperClientCpp VERSION 0.1.0)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add Qt installation to CMAKE_PREFIX_PATH
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.8.0/msvc2022_64")

# Find required packages
find_package(Qt6 COMPONENTS Core Widgets WebSockets REQUIRED)

# Fetch external dependencies
include(FetchContent)

# Whisper.cpp
FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG master
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(whisper)

# RtAudio for audio capture
FetchContent_Declare(
    rtaudio
    GIT_REPOSITORY https://github.com/thestk/rtaudio.git
    GIT_TAG master
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rtaudio)

# JSON for Modern C++
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(external)
enable_testing()
add_subdirectory(test)

# Install targets
install(TARGETS whisper-client
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Automatically copy Qt DLLs to output directory
if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    
    # List of Qt components that need to be deployed
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT qt-runtime)
    install(
        DIRECTORY "${_qt_bin_dir}/"
        DESTINATION bin
        CONFIGURATIONS Release
        FILES_MATCHING 
        PATTERN "Qt6Core*.dll"
        PATTERN "Qt6Gui*.dll"
        PATTERN "Qt6Widgets*.dll"
        PATTERN "Qt6WebSockets*.dll"
        PATTERN "Qt6Network*.dll"  # Required by WebSockets
    )
endif()