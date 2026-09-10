cmake_minimum_required(VERSION 3.5)

# 添加Nes目录中的源文件
add_library(Nes STATIC
    Bus.cpp
    Bus.h
    Cartridge.cpp
    Cartridge.h
    Cpu.cpp
    Cpu.h
    Nes.cpp
    Nes.h
    NesClock.cpp
    NesClock.h
    Ppu.cpp
    Ppu.h
    Mapper/Mapper.cpp
    Mapper/Mapper.h
    Mapper/Mapper000.cpp
    Mapper/Mapper000.h
    Mapper/eeprom/x24c01.cpp
    Mapper/eeprom/x24c01.h
    Mapper/eeprom/x24c02.cpp
    Mapper/eeprom/x24c02.h
    Ram.cpp
    Ram.h
)

target_include_directories(Nes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Nes 核心只需要 Qt Core 类型，不依赖 Widgets。
target_link_libraries(Nes PRIVATE Qt${QT_VERSION_MAJOR}::Core)