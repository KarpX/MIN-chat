cmake_minimum_required(VERSION 3.16)
project(ChatClient)

find_package(Qt6 REQUIRED COMPONENTS Quick Network Core QuickControls2)

set(CLIENT_SOURCES
    main.cpp
    ChatController.h
    ChatController.cpp
    NetworkClient.cpp
    NetworkClient.h
    ConnectionStates.cpp
    ConnectionStates.h
)

qt_add_executable(ChatClient ${CLIENT_SOURCES})

qt_add_qml_module(ChatClient
    URI ChatClient
    RESOURCE_PREFIX "/"
    QML_FILES main.qml
    SOURCES
)

target_link_libraries(ChatClient PRIVATE Qt6::Quick Qt6::Network Qt6::Core Qt6::QuickControls2 CommonLib)

target_include_directories(ChatClient PRIVATE ../common)