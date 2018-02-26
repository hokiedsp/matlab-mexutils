# matlab-mexutils

C++ utility headers for Matlab MEX development + cmake template

This respository contains various utilities for Matlab MEX development in C++:

* Matlab/C++ Class Unification
* Other Utility header Files
* CMake MEX Build/Install Process

## Matlab/C++ class Unification:

### `include/mexClassHandler.h` & `+mexcpp/BaseClass.m`

This portion is based on [Oliver Woodford's implementation of a MATLAB class to wrap a C++ class](https://www.mathworks.com/matlabcentral/fileexchange/38964). Three core templates are found in 
mexClassHandler.h:

Type | Name | Description
-----|------|-------------
function|`mexClassHandler`|All-in-one template function to be called in mexFunction
class|`mexClassHandle`|Implements C++ object wrapping mechanism. This class is transparent in a `mexClassHandler`-based MEX function.
class|`mexSetGetClass`|Base class with set/get/save/load actions

`mexClassHandler` should be paired with the MATLAB base/template handle class `+mexcpp/BaseClass.m`. In general, the paired MATLAB class must have a property `backend` with attributes: `(Access = protected, Hidden, NonCopyable, Transient)`. The class should also declare the MEX function as its static member. This minimizes erroneous call to the MEX function. In `+mexcpp/BaseClass.m`, the MEX function is declared as `mexfcn`, and note that the MEX target name in `examples/@mexClass_demo/CMakeLists.txt` is also `mexfcn`.

To implement a MEX function with this framework, `mexFunction` would require a single line:
```
#include <mex.h>
#include "mexClassHandler.h"

class myClass;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  mexClassHandler<myClass>(nlhs, plhs, nrhs, prhs);
}
```
Here, `myClass` is the target C++ class to be wrapped by the MATLAB class.

There are several requirements to specialize `mexClassHandler` for `myClass`. First, it must have a constructor with signature:
```
myClass(int nrhs, const mxArray *prhs[]);
```
The constructor arguments are passed in directly from MATLAB as described below. `myClass` must also define the following public functions:

C++ Signature | Description
----------|------------
 `static std::string get_classname()`|Return the name of paired MATLAB class
`bool action_handler(const mxArray *mxObj, const std::string &action, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]`)|Perform the specified action
`static bool static_handler(std::string action, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])`|Perform the specified *static* action

In MATLAB, the static MEX member function `mexfcn` maybe used with 4 signatures in the MATLAB class:

MATLAB Signature | Description
-|-
`mexfcn(obj, varargin)`| Create a new C++ class instance and store it as the `backend` MATLAB class property. Errors out if `backend` is not empty.
`mexfcn(obj, 'delete')`| Destruct the C++ class instance pointed by the `backend` property
`varargout = mexfcn(obj, action, varargin)` | Perform specified action of the wrapped C++ object
`varargout = mexfcn('action', varargin)` | Perform specified *static* action of the wrapped C++ object

The arguments `int nlhs`, `mxArray *plhs[]`, `int nrhs`, and `const mxArray *prhs[]` in `action_handler()` and `static_handler()` carry `varargout` and `varargin` of their corresponding MATLAB calls.

Accessing member variables of the C++ backend object from MATLAB is often important, and `mexSetGetClass` implements `set` and `get` actions, which call the derived class' `set_prop` and `get_prop`, respectively. Note that this implementation is not the most efficient but may be useful to separate the set/get actions from other actions for a large-scale class object. In addition to set/get, `load` and `save` actions are suggested to be used with `saveobj` and `loadobj` MATLAB class functions.

## Other utility header files:

### `include/mexRuntimeError.h`

Derived class of C++ `std::runtime_error` to log the exception `id` in addition so that `mexClassHandler()` can catch the exception and call `mexErrMsgIdAndTxt()`.

### `include/mexGetString.h`

Defines a function `mexGetString()`, which converts `char`/`cellstr` `mxArray` to `std::string`. This function is based on `mxGetString()` LIBMX function and only supports *single-byte* character strings.

### `include/mexAllocator.h`

Defines `mexAllocator` class, which is a custom C++ allocator. It wraps `mxCalloc()`, `mxRealloc()`, and `mxFree()`. This allocator is useful to write a template class, which dynamically allocates memory and the allocated memory is later used in Matlab (i.e., set to an `mxArray` object) completely detached from the template class. Such template class could be written independent of Matlab with an Allocator template.

## Building MEX function with CMake

[CMake](http://cmake.org) is one of the most widely used cross-platform build automation software, and it meshes well with MATLAB, which is also a cross-platform environment. While `mex` command in MATLAB does exactly that, configuring it becomes cumbersome quickly as the scale of the project grows. The couple features of CMake makes it very attractive platform to build MEX functions:

* Built-in support to configure header and library files via `find_package(Matlab)` command, which runs `Modules/FindMatlab.cmake`.
* `matlab_add_mex` macro  in `Modules/FindMatlab.cmake` to create and configure the MEX target.
* Target installation. The built targets as well as any files in the project can be made to be "installed" to a separate directory.

This repository presents a way to set up CMake for a MEX project,utilizing these features. For the first two points, readers are referred to the CMake documentation for the details.

The installation process of CMake is what makes it a great MEX development environment. Here is how to set up a MEX project.

1. Create a MEX project folder, which only holds the related m-files (e.g., help text file for a mex function or other class functions for a mex class) and C/C++ source files.
   * The directory structure should reflect how files will be installed. For example, have `+package` folders to define a MATLAB package or `@class` folders to define a MATLAB class with multiple m-files.
   * Place `mexFunction` C/C++ source files at exactly where the compiled MEX files should be placed. For example, the source file for the `mexfcn` MEX function for the `myClass` example above should be placed inside of `@myClass` folder.
2. CMake build process takes the C/C++ source files and place the compiled binary files in the build directory.
3. CMake installation process can be set to copy all the m-files in the MEX project directory to the installation location, keeping the directory structure intact. Then, make it copy all the MEX binaries to the appropriate place in the install directory (same relative locations as the source files in the project directory).

This keeps all the non-Matlab files (i.e., source files and any intermediate build files) away from the final product while presenting the m-files and C/C++ files in the easy-to-relate locations.

### Note on building MEX function with MS Visual C++ compiler

While the Mathwork's documentation suggests using a `def` file to export `mexFunction` symbol, it could be achieved simply with a linker flag `/EXPORT:mexFunction`. In CMake, this flag can be set by the command:

```
set_target_properties(${MEX_TARGET} PROPERTIES LINK_FLAGS /EXPORT:mexFunction)
```