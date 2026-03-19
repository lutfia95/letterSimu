cmake_minimum_required(VERSION 3.16)

project(letterSimu VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lettersimu_config
    src/ConfigReader.cpp
    src/DnaReference.cpp
    src/RnaTranscriptome.cpp
    src/DnaReads.cpp
    src/RnaReads.cpp
)

target_include_directories(lettersimu_config
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(lettersimu
    src/main.cpp
)

target_link_libraries(lettersimu
    PRIVATE
        lettersimu_config
)
