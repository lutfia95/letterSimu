cmake_minimum_required(VERSION 3.16)

project(letterSimu VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    sharg
    GIT_REPOSITORY https://github.com/lutfia95/sharg-parser.git
    GIT_TAG main
)
FetchContent_MakeAvailable(sharg)

add_library(lettersimu_config
    src/ConfigReader.cpp
    src/DnaReference.cpp
    src/RnaTranscriptome.cpp
    src/DnaReads.cpp
    src/RnaReads.cpp
    src/Logger.cpp
)

target_include_directories(lettersimu_config
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lettersimu_config
    PUBLIC
        spdlog::spdlog
)

add_executable(lettersimu
    src/main.cpp
)

target_link_libraries(lettersimu
    PRIVATE
        lettersimu_config
        sharg::sharg
)
