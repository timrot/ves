if("${CMAKE_CXX_COMPILER}" MATCHES "qcc")
  message("Setting null")
  set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")
  set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
endif()
