
FIND_PACKAGE(PythonInterp)
FIND_PACKAGE(PythonLibs)

IF (PYTHONINTERP_FOUND AND PYTHONLIBS_FOUND)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

  OPTION(BUILD_SHARED_LIBS "turn OFF for .a libs" ON)

  ADD_LIBRARY(pystinger _stinger.c pystinger.c)
  SET_TARGET_PROPERTIES(pystinger PROPERTIES PREFIX "")
  SET_TARGET_PROPERTIES(pystinger PROPERTIES SUFFIX ".so")
  ADD_DEPENDENCIES(pystinger stinger_core stinger_utils)
  TARGET_LINK_LIBRARIES(pystinger stinger_core stinger_utils python)
ELSE()
  MESSAGE("Could not find Python.  Skipping vquery extension.")
ENDIF()
