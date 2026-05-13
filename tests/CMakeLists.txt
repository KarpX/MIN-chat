cmake_minimum_required(VERSION 3.16)
project(ChatTests)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(UnitTests 
    main_test.cpp 
    Test_Crypto.cpp 
    Test_Factory.cpp
    Test_Database.cpp
    Test_Scenarios.cpp
)

target_link_libraries(UnitTests PRIVATE 
    gtest_main 
    CommonLib 
    Qt6::Core 
    Qt6::Network
)

include(GoogleTest)
gtest_add_tests(TARGET UnitTests)