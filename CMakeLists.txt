cmake_minimum_required(VERSION 3.5.0)
project(chip8 VERSION 0.1)

find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})


configure_file(config.h.in config.h)

set(CMAKE_CXX_STANDARD 20)

set(BASE_FILES  ${PROJECT_SOURCE_DIR}/src/machine.cpp /
                ${PROJECT_SOURCE_DIR}/src/instructions.cpp /
  # ${PROJECT_SOURCE_DIR}/src/logger/logger.cpp /
  # ${PROJECT_SOURCE_DIR}/src/datasrc/datasrc.cpp /
  # ${PROJECT_SOURCE_DIR}/src/datasrc/procfs.cpp /
  # ${PROJECT_SOURCE_DIR}/src/datasrc/shellcmd.cpp /
)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

add_executable(chip8 ${PROJECT_SOURCE_DIR}/src/chip8.cpp)
target_sources(chip8 PUBLIC ${BASE_FILES})
target_link_libraries(chip8 ${SDL2_LIBRARIES})

target_include_directories(chip8 PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
