add_library (wordcount MODULE wordcount.cpp)
set_target_properties(wordcount PROPERTIES PREFIX "")

add_library (numvalue MODULE wordsort.cpp)
set_target_properties(numvalue PROPERTIES PREFIX "")

install (TARGETS
  wordcount
  numvalue
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/mapredo)
