# compile back-end mex function for mexClass_demo class
set(MEX_FILE "mexfcn")
set(MEX_FILE_NAME "mexClass_demo_backend.cpp")

matlab_add_mex(NAME ${MEX_FILE} SRC ${MEX_FILE_NAME})

# if the compiler is MS VisualC++, add 'mexFunction' to the exported symbols
if(WIN32 AND MSVC)
  set_target_properties(${MEX_FILE} PROPERTIES LINK_FLAGS /EXPORT:mexFunction)
endif()

# install
file(RELATIVE_PATH DstRelativePath "${CMAKE_SOURCE_DIR}/examples" ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS ${MEX_FILE} RUNTIME DESTINATION "${DstRelativePath}")
