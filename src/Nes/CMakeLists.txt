cmake_minimum_required(VERSION 3.5)

# 添加Nes目录中的源文件
add_library(Nes STATIC
    Cpu.cpp
    Cpu.h
    Mmu.cpp
    Mmu.h
    Ppu.cpp
    Ppu.h
    Rom.cpp
    Rom.h
    Mapper/eeprom/x24c01.cpp
    Mapper/eeprom/x24c01.h
    Mapper/eeprom/x24c02.cpp
    Mapper/eeprom/x24c02.h
    Ram.cpp
    Ram.h
)

target_include_directories(Nes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接Qt库
find_package(Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
target_link_libraries(Nes PRIVATE Qt5::Widgets)