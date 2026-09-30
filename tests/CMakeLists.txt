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