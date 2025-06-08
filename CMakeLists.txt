cmake_minimum_required(VERSION 3.5)

project(NesV3 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES i18n/NesV3_en_US.ts)

set(PROJECT_SOURCES
	    src/main.cpp
		src/MainWindow.cpp
		src/MainWindow.h
		${TS_FILES}
        src/Excption/OutOfRangeError.cpp
        src/Excption/OutOfRangeError.h
		src/Excption/ValueError.cpp
		src/Excption/ValueError.h
		src/Excption/BaseError.cpp
		src/Excption/BaseError.h
		src/Excption/FileError.cpp
		src/Excption/FileError.h
		src/Excption/IOError.cpp
		src/Excption/IOError.h
		src/Excption/RuntimeError.cpp
		src/Excption/RuntimeError.h
)

# 添加Nes子目录
add_subdirectory(src/Nes)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
	qt_add_executable(NesV3
		MANUAL_FINALIZATION
		${PROJECT_SOURCES}
	)
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET NesV3 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
	if(ANDROID)
		add_library(NesV3 SHARED
			${PROJECT_SOURCES}
		)
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
		add_executable(NesV3
			${PROJECT_SOURCES}
		)
    endif()

	qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(NesV3 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Nes)

set_target_properties(NesV3 PROPERTIES
	MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
	MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
	MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
	MACOSX_BUNDLE TRUE
	WIN32_EXECUTABLE TRUE
)

install(TARGETS NesV3
	BUNDLE DESTINATION .
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
	qt_finalize_executable(NesV3)
endif()