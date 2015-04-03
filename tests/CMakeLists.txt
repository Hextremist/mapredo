
add_executable(unittests
  buffer_trader.cpp
  data_reader.cpp
  plugin.cpp
  test.cpp
  ../mapredo/directory.cpp
  ../mapredo/buffer_trader.cpp
  ../mapredo/errno_message.cpp)

target_link_libraries(unittests
  ${GTEST_BOTH_LIBRARIES}
  snappy
  ${CMAKE_DL_LIBS})
include_directories(../mapredo)

add_test (all_tests unittests)
