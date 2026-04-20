project(ChatServer)

set(SERVER_SOURCES
    main.cpp
    ChatServer.cpp
    ChatServer.h
)

add_executable(ChatServer ${SERVER_SOURCES})

target_link_libraries(ChatServer PRIVATE
    Qt6::Network
    CommonLib
)

target_include_directories(ChatServer PRIVATE ../common)