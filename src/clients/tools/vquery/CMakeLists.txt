
FIND_PACKAGE(PythonInterp)
FIND_PACKAGE(PythonLibs)

IF (PYTHONINTERP_FOUND AND PYTHONLIBS_FOUND AND NOT CYGWIN)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

  OPTION(BUILD_SHARED_LIBS "turn OFF for .a libs" ON)

  ADD_LIBRARY(pystinger _stinger.c pystinger.c)
  SET_TARGET_PROPERTIES(pystinger PROPERTIES PREFIX "")
  ADD_DEPENDENCIES(pystinger stinger_core stinger_utils)
  TARGET_LINK_LIBRARIES(pystinger stinger_core stinger_utils)
  IF (APPLE)
    TARGET_LINK_LIBRARIES(pystinger python)
    SET_TARGET_PROPERTIES(pystinger PROPERTIES SUFFIX ".so")
  ENDIF()
ELSE()
  MESSAGE("Could not find Python.  Skipping vquery extension.")
  IF(CYGWIN)
  MESSAGE("Uglyness, but the vquery python stinger stuff is not supported on Cygwin for now.")
  ENDIF()
ENDIF()
