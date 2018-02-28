# matlab-mexutils

C++ Utility Headers for Matlab MEX Development + CMake Template

Also listed on Matlab File Exchange at http://www.mathworks.com/matlabcentral/fileexchange/66217

This respository contains various utilities for Matlab MEX development in C++:

* [Unification of Matlab & C++ Classs](#unification-of-matlab--c-classs)
* [Other Utility Header Files](#other-utility-header-files)
* [Building MEX Functions with CMake](#building-mex-functions-with-cmake)

## Demo Installation

To build and install the examples included in this repo, use CMake + your favorite C++ compiler. By default, the CMake installation process creates `mexcpp_demo` folder in your MATLAB user directory (e.g., `My Documents/MATLAB/mexcpp_demo` in Windows). To change the installation destination, modify `MEXCPP_DEMO_INSTALL_DIR` cache variable (not `CMAKE_INSTALL_PREFIX`). Note that the installation process is required to bring m-files and mex-files together.

## Unification of Matlab & C++ Classs

### [`include/mexObjectHandler.h`](include/mexObjectHandler.h)

This portion is based on [Oliver Woodford's implementation of a MATLAB class to wrap a C++ class](https://www.mathworks.com/matlabcentral/fileexchange/38964). Three templates are found in 
[mexObjectHandler.h](include/mexObjectHandler.h):

Type | Name | Description
-----|------|-------------
function|`mexObjectHandler`|All-in-one template function to be called in mexFunction
class|`mexObjectHandle`|Implements C++ object wrapping mechanism. This class is transparent in a `mexObjectHandler`-based MEX function.
class|`mexSetGetClass`|Base class with set/get/save/load actions

`mexObjectHandler` is designed to be paired with the MATLAB base/template handle class [`mexcpp.BaseClass`](#mexcppbaseclassm). See the section below for the specifications of the m-file.

To implement a MEX function with this framework, `mexFunction` would require a single line:

```c++
#include <mex.h>
#include "mexObjectHandler.h"

class myClass;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  mexObjectHandler<myClass>(nlhs, plhs, nrhs, prhs);
}
```

Here, `myClass` is the target C++ class to be wrapped by the MATLAB class.

There are several requirements to specialize `mexObjectHandler` for `myClass`. First, it must have a constructor with signature:

```c++
myClass(const mxArray *mxObj, int nrhs, const mxArray *prhs[]);
```

The first argument `mxObj` points to the MATLAB class object, and `nrhs` and `prhs` gets the `varargin` constructor arguments are passed in directly from MATLAB as described below. `myClass` must also define the following public functions:

C++ Signature | Description
----------|------------
`static std::string get_classname()`|Return the name of paired MATLAB class
`bool action_handler(const mxArray *mxObj, const std::string &action, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]`)|Perform the specified action
`static bool static_handler(std::string action, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])`|Perform the specified *static* action

In MATLAB, the static MEX member function (its default name: `mexfcn`) maybe used with 4 signatures in the MATLAB class:

MATLAB Signature | Description
---|---
`mexfcn(obj, varargin)`| Create a new C++ class instance and store it as the `backend` MATLAB class property. Errors out if `backend` is not empty.
`mexfcn(obj, 'delete')`| Destruct the C++ class instance pointed by the `backend` property
`varargout = mexfcn(obj, action, varargin)` | Perform specified action of the wrapped C++ object
`varargout = mexfcn('action', varargin)` | Perform specified *static* action of the wrapped C++ object

The arguments `int nlhs`, `mxArray *plhs[]`, `int nrhs`, and `const mxArray *prhs[]` in `action_handler()` and `static_handler()` carry `varargout` and `varargin` of their corresponding MATLAB calls.

### [`+mexcpp/BaseClass.m`](+mexcpp/BaseClass.m)

This abstract class is a bare-bone *handle* class to be paird with a MEX function running the `mexObjectHandler()` template function to wrap a C++ backend class instance.

It features:

* Protected `backend` property to hold a mexObjectHandle
* Abstract protected static method `varargout = mexfcn(varargin)` to reserve the MEX function as its protected method
* Constructor calls `obj.mexfcn(obj, varargin{:})` to create a pairing C++ object (the mexFunction implicitly store it in the `backend` property)
* Deleter calls `obj.mexfcn(obj, 'delete')` to destroy the backend C++ object

Subclass inheriting `mex.BaseClass` must:

* Pass necessary input arguments to `mexcpp.BaseClass` constructor to instantiate the C++ object
* Although accessible, leave `backend` property alone (unless the backend needs to be recreated)
* Implement various interface functions to perform C++ actions via mexFunction calls: `[...] = obj.mexfcn(obj, 'action', ...)` or statically `[...] = obj.mexfcn('action', ...)`
* Use ASCII action names as `mexObjectHandler()` does not accept multi-byte characters for the action name

This MATLAB class could be used as the superclass for a user's wrapper class as outline thus far, or it can be used as a template for a standalone class. Such class **must be a handle class**.

### [`include/mexSetGetClass.m`](include/mexSetGetClass.m)

Accessing member variables of the C++ backend object from MATLAB is often important, and `mexSetGetClass` implements `set` and `get` actions, which call the derived class' `set_prop` and `get_prop`, respectively. Note that this implementation is not the most efficient but may be useful to separate the set/get actions from other actions for a large-scale class object. In addition to set/get, `load` and `save` actions are suggested to be used with `saveobj` and `loadobj` MATLAB class functions.

### Standalone Usage of `mexObjectHandle` Template Class

`mexObjectHandle` may be used on its own without `mexObjectHandler()`. See [`examples/mexCounter.cpp`](examples/mexCounter.cpp) and [`examples/mexCounter_demo.m`](examples/mexCounter_demo.m) for such an example. Note that the wrapped C++ "object" in this example is a plain integer to store the counter state. This demo also demonstrates that you can have multiple handles of the same MEX function. Last, *Use `onCleanup` class in MATLAB to guarantee that the MEX object gets deleted when MATLAB workspace is cleared.* As illustrated in the demo, the handle stored in a MATLAB variable could easily be overwritten and without the `onCleanup` mechanism, the C++ object gets completely lost and the lock on the MEX function will never be removed.

## Other Utility Header Files

### [`include/mexRuntimeError.h`](include/mexRuntimeError.h)

Derived class of C++ `std::runtime_error` to log the exception `id` in addition so that `mexObjectHandler()` can catch the exception and call `mexErrMsgIdAndTxt()`.

### [`include/mexGetString.h`](include/mexGetString.h)

Defines a function `mexGetString()`, which converts `char`/`cellstr` `mxArray` to `std::string`. This function is based on `mxGetString()` LIBMX function and only supports *single-byte* character strings.

### [`include/mexAllocator.h`](include/mexAllocator.h)

Defines `mexAllocator` class, which is a custom C++ allocator. It wraps `mxCalloc()`, `mxRealloc()`, and `mxFree()`. This allocator is useful to write a template class, which dynamically allocates memory and the allocated memory is later used in Matlab (i.e., set to an `mxArray` object) completely detached from the template class. Such template class could be written independent of Matlab with an Allocator template.

## Building MEX Functions with CMake

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
