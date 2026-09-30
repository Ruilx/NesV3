add_executable(NesCoreTests
    NesCoreTests.cpp
)

target_link_libraries(NesCoreTests PRIVATE Nes Qt${QT_VERSION_MAJOR}::Core)

add_test(NAME NesCoreTests COMMAND NesCoreTests)

add_executable(BusTests
    BusTests.cpp
)

target_link_libraries(BusTests PRIVATE Nes Qt${QT_VERSION_MAJOR}::Core)

add_test(NAME BusTests COMMAND BusTests)

add_executable(ApuTests
    ApuTests.cpp
)

target_link_libraries(ApuTests PRIVATE Nes Qt${QT_VERSION_MAJOR}::Core)

add_test(NAME ApuTests COMMAND ApuTests)

add_executable(AudioSampleSchedulerTests
    AudioSampleSchedulerTests.cpp
)

target_link_libraries(AudioSampleSchedulerTests PRIVATE Nes Qt${QT_VERSION_MAJOR}::Core)

add_test(NAME AudioSampleSchedulerTests COMMAND AudioSampleSchedulerTests)

add_executable(AudioOutputShaperTests
    AudioOutputShaperTests.cpp
)

target_link_libraries(AudioOutputShaperTests PRIVATE Nes Qt${QT_VERSION_MAJOR}::Core)

add_test(NAME AudioOutputShaperTests COMMAND AudioOutputShaperTests)

add_executable(AudioRingBufferTests
    AudioRingBufferTests.cpp
)

target_link_libraries(AudioRingBufferTests PRIVATE Nes Qt${QT_VERSION_MAJOR}::Core)

add_test(NAME AudioRingBufferTests COMMAND AudioRingBufferTests)

add_executable(AudioOutputDeviceTests
    AudioOutputDeviceTests.cpp
    ${CMAKE_SOURCE_DIR}/src/AudioOutputDevice.cpp
)

target_include_directories(AudioOutputDeviceTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(AudioOutputDeviceTests PRIVATE Nes Qt${QT_VERSION_MAJOR}::Core)

add_test(NAME AudioOutputDeviceTests COMMAND AudioOutputDeviceTests)