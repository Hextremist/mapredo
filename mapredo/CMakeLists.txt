
set (sourcefiles
  base.cpp
  buffer_trader.cpp
  consumer.cpp
  directory.cpp
  engine.cpp
  errno_message.cpp
  file_merger.cpp
  settings.cpp
  sorter_buffer.cpp
  sorter.cpp)
if(NOT MSVC)
  list(APPEND sourcefiles directory.cpp)
else()
  list(APPEND sourcefiles directory_win32.cpp)
endif()

add_library (lmapredo SHARED ${sourcefiles})
set(lmapredo_VERSION_STRING 0.0.1)

set_target_properties(lmapredo PROPERTIES
  VERSION ${lmapredo_VERSION_STRING}
  SOVERSION 0
  OUTPUT_NAME mapredo)

target_link_libraries (lmapredo pthread snappy)

set (headerfiles
  base.h
  config.h
  config_param.h
  engine.h
  mapreducer.h
  settings.h)
if(NOT MSVC)
  list(APPEND headerfiles directory.h)
else()
  list(APPEND headerfiles directory_win32.h)
endif()

install (FILES ${headerfiles} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mapredo)
install (TARGETS lmapredo DESTINATION ${CMAKE_INSTALL_LIBDIR})
