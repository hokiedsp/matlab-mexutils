cmake_minimum_required(VERSION 2.4.0) 
# for FindMatlab support (may require a later version to detect the latest Matlab release)

project(matlab-mexutils)

option(MatlabMexutils_BuildExamples "Turn on to build example MEX files")
option(MatlabMexutils_BuildDocs "Turn on to build Doxygen documentation")

# get the MATLAB user folder (par MATHWORKS website)
if (WIN32)
  set(MATLAB_USER_DIR "$ENV{USERPROFILE}/Documents/MATLAB")
else()
  set(MATLAB_USER_DIR "$ENV{home}/Documents/MATLAB")
endif()

# Look for MATLAB API library paths
find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY)
# Libmex library included by default.
# This demo target only requires MX_LIBRARY (libmx) in addition
# add other components as needed. See Modules/FindMatlab.cmake or it 
# documentation for other supported components. 

get_filename_component(Matlab_RELEASE ${Matlab_ROOT_DIR} NAME)
matlab_get_version_from_release_name(${Matlab_RELEASE} Matlab_VERSION)
if (Matlab_VERSION AND Matlab_VERSION VERSION_LESS "8.5")
  add_compile_definitions(MATLAB_PRE_R2015A)
endif()

# turn off new Matlab class support
set(Matlab_HAS_CPP_API 0)

# interface library for mex related files
add_library(libmex INTERFACE)
target_include_directories(libmex INTERFACE ${Matlab_INCLUDE_DIRS})
target_link_libraries(libmex INTERFACE ${Matlab_MEX_LIBRARY} ${Matlab_MX_LIBRARY})

# header-only interface mexutils library
add_library(libmexutils INTERFACE)
target_include_directories(libmexutils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(libmexutils INTERFACE libmex)

if (MatlabMexutils_BuildExamples)
# Set the installation directory if not already given in cache
if (NOT MEXCPP_DEMO_INSTALL_DIR)
  # By default, installer creates `mexcpp_demo` folder in the 
  # default MATLAB folder in the user account folder.

  # Append `mexcpp_demo` to the dir
  if (NOT EXISTS ${MATLAB_USER_DIR})
    # if not found, show message and use the default CMake install directory
    message("Matlab user directory not found at ${MATLAB_USER_DIR}")
    message("Default CMake installation path used.")
    set(MEXCPP_DEMO_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
  else()
    set(MEXCPP_DEMO_INSTALL_DIR "${MATLAB_USER_DIR}/mexcpp_demo")
  endif()

  # put MEXCPP_DEMO_INSTALL_DIR in cache to allow user modification
  set(MEXCPP_DEMO_INSTALL_DIR ${MEXCPP_DEMO_INSTALL_DIR} CACHE PATH "Matlab MEXCPP Demo File Installation Directory")
endif()

# force to change the CMake install folder
set(CMAKE_INSTALL_PREFIX ${MEXCPP_DEMO_INSTALL_DIR} CACHE PATH "Install path prefix, prepended onto install directories." FORCE)

# add the install targets
install(DIRECTORY . DESTINATION . FILES_MATCHING PATTERN "*.m") # only copy m files and ignore source files

# traverse subfolders
add_subdirectory(examples)

endif()

# first we can indicate the documentation build as an option and set it to ON by default
# documentation build
if (MatlabMexutils_BuildDocs)
  set(BUILD_DOC ON)
  find_package(Doxygen) # check if Doxygen is installed
  if (DOXYGEN_FOUND AND CMAKE_BUILD_TYPE MATCHES "^[Rr]elease")
      # set input and output files
      set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
      set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

      # request to configure the file
      configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
      message("Doxygen build started")

      # note the option ALL which allows to build the docs together with the application
      add_custom_target( doc_doxygen ALL
          COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          COMMENT "Generating API documentation with Doxygen"
          VERBATIM )
  else ()
    message("Doxygen need to be installed to generate the doxygen documentation")
  endif ()
endif()