# copy all the m-files to the demo folder
install(DIRECTORY . DESTINATION . FILES_MATCHING PATTERN "*.m") # only copy m files and ignore source files

# go to the subdirectory to build the mexfcn
add_subdirectory(@mexClass_demo)
