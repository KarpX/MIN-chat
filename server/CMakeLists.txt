project(ChatServer)

find_package(Qt6 REQUIRED COMPONENTS Network)

add_executable(ChatServer
    main.cpp
    ChatServer.h
)

target_link_libraries(ChatServer PRIVATE
    Qt6::Network
    CommonLib
)