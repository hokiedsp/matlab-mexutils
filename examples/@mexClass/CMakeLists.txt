# compile back-end MEX function for mexClass class
set(MEX_FILE "mexfcn") # name of MEX file
set(MEX_FILE_NAME "mexClass_mexfcn.cpp") # source file defining mexFunction()

matlab_add_mex(NAME ${MEX_FILE} SRC ${MEX_FILE_NAME})
target_link_directories(${MEX_FILE} PRIVATE matlab-mexutils)

# install
file(RELATIVE_PATH DstRelativePath "${CMAKE_SOURCE_DIR}/examples" ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS ${MEX_FILE} RUNTIME DESTINATION "${DstRelativePath}")
