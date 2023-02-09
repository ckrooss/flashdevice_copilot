cmake_minimum_required(VERSION 3.25)
project(main)

find_package(GTest)
set(CMAKE_CXX_STANDARD 17)
enable_testing()

add_executable(main_test
test.cpp)

target_link_libraries(main_test GTest::GTest)

add_test(
	NAME main_test
	COMMAND main_test
)