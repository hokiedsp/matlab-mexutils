# matlab-mexutils
C++ utility headers for Matlab MEX development + cmake template

This respository contains various utilities for Matlab MEX development in C++:
* Matlab/C++ class unification
* Utility header files
* CMake template

## Matlab/C++ class unification:
### `include/mexClassHandler.h` + `+mexcpp/BaseClass.m`
This is based on [Oliver Woodford's implementation of a MATLAB class to wrap a C++ class](https://www.mathworks.com/matlabcentral/fileexchange/38964).
He casts a pointer to C++ class object to `uint64_t` and stores it as a MATLAB `uint64` variable. By making the casted `uint64`
variable a MATLAB class property, the scope of the underlying C++ class object is concretely defined (i.e., instantiated during 
MATLAB class construction and destructed during MATLAB class deletion). Using his wrapping mechanism (`mexClassHandle`), 

