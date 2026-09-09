add_executable(NesCoreTests
    NesCoreTests.cpp
)

target_link_libraries(NesCoreTests PRIVATE Nes Qt${QT_VERSION_MAJOR}::Core)

add_test(NAME NesCoreTests COMMAND NesCoreTests)