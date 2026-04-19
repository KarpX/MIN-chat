project(CommonLib)

add_library(CommonLib STATIC
    MessageModel.h
    IEncryptionStrategy.h
    AES256GCMStrategy.h
    CryptoManager.h
    INetworkObserver.h
    IConnectionState.h
    ConnectionStates.h
    DatabaseManager.h
)

target_link_libraries(CommonLib PUBLIC OpenSSL::Crypto)

find_package(SQLite3 REQUIRED)
target_link_libraries(CommonLib PUBLIC SQLite::SQLite3)