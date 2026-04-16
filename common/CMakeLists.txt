project(CommonLib)

add_library(CommonLib STATIC
    MessageModel.h
    IEncryptionStrategy.h
    AES256GCMStrategy.h
    CryptoManager.h
    INetworkObserver.h
    IConnectionState.h
    ConnectionStates.h
)
target_include_directories(CommonLib PUBLIC .)