cmake_minimum_required(VERSION 3.22)

# Set the project name and language
project(RTS2 C)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Find ALSA
find_package(ALSA REQUIRED)

# Find SpeexDSP via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SPEEXDSP REQUIRED speexdsp)

# Define the executable and its sources
add_executable(${PROJECT_NAME})

# Add sources to the executable
target_sources(${PROJECT_NAME} PRIVATE
    src/main.c
    src/audioIO.c
    src/fifo.c
    src/audioProcessing.c
)

# Add include paths
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
    ${ALSA_INCLUDE_DIRS}  # Add ALSA includes
    ${SPEEXDSP_INCLUDE_DIRS}  # SpeexDSP includes
)

# Link ALSA and other libs
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${ALSA_LIBRARIES}
    ${SPEEXDSP_LIBRARIES}
    m
    pthread
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -O2
)