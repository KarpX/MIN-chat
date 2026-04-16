project(ChatClient)

find_package(Qt6 REQUIRED COMPONENTS Quick Network)

set(SOURCES
    main.cpp
    ChatController.h
    NetworkClient.h
    NetworkClient.cpp
    main.qml
)

qt_add_executable(ChatClient ${SOURCES})

qt_add_qml_module(ChatClient
    URI ChatClient
    RESOURCE_PREFIX "/"
    QML_FILES main.qml
)

target_link_libraries(ChatClient PRIVATE
    Qt6::Quick
    Qt6::Network
    CommonLib
)

target_include_directories(ChatClient PRIVATE ../common)