cmake_minimum_required(VERSION 2.4.0) 
# for FindMatlab support (may require a later version to detect the latest Matlab release)

# Look for MATLAB API library paths
find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY)
# Libmex library included by default.
# This demo target only requires MX_LIBRARY (libmx) in addition
# add other components as needed. See Modules/FindMatlab.cmake or its
# documentation for other supported components. 

# Add the matlab-mexutils' include folder to access the mex utility header files
include_directories("include")

# set globally to ignore MSVC warning on STD library calls
if(WIN32 AND MSVC)
  add_compile_options(/wd4996)
endif()

# Set the installation directory if not already given in cache
if (NOT MEXCPP_DEMO_INSTALL_DIR)
  # By default, installer creates `mexcpp_demo` folder in the 
  # default MATLAB folder in the user account folder.

  # get the MATLAB user folder (par MATHWORKS website)
  if (WIN32)
    set(MATLAB_USER_DIR "$ENV{USERPROFILE}/Documents/MATLAB")
  else()
    set(MATLAB_USER_DIR "$ENV{home}/Documents/MATLAB")
  endif()

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
install(DIRECTORY +mexcpp DESTINATION .)

# first we can indicate the documentation build as an option and set it to ON by default
option(BUILD_DOC "Build documentation" ON)

# documentation build
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


# traverse subfolders
add_subdirectory(examples)
