cmake_minimum_required(VERSION 3.16)
project(CommonLib)
find_package(Qt6 REQUIRED COMPONENTS Core Network)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
set(COMMON_SOURCES
    AES256GCMStrategy.cpp
    AES256GCMStrategy.h
    DatabaseManager.cpp
    DatabaseManager.h
    CryptoManager.cpp
    CryptoManager.h
    IEncryptionStrategy.h
    IEncryptionStrategy.cpp
    INetworkObserver.h
    MessageModel.h
    ContactModel.h
)
add_library(CommonLib STATIC ${COMMON_SOURCES}
    IConnectionState.h)
target_link_libraries(CommonLib PUBLIC Qt6::Core Qt6::Network OpenSSL::Crypto SQLite::SQLite3)
target_include_directories(CommonLib PUBLIC .)