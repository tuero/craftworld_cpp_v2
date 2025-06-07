add_executable(craftworld_test test.cpp)
target_link_libraries(craftworld_test PUBLIC craftworld)
add_test(craftworld_test craftworld_test)

