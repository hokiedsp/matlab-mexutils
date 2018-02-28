/** \file mexObjectHandler.h
 * C++ header file containing classes and function necessary to wrap C++ class with MATLAB class
 */

#pragma once

#include "mexGetString.h"    // to convert char mexArray to std::string
#include "mexRuntimeError.h" // for mexRuntimeError runtime exception class

#include <mex.h>
#include <stdint.h>
#include <typeinfo>

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

/**
 * \brief Underlying wrapper class to wrap C++ object by an mxArray object
 * 
 * mexObjectHandle is a class template to wrap a C++ object so that the object can
 * be preserved over multiple mexFunction calls while allowing the creation of multiple
 * objects unlike using static variables. 
 * 
 * The original credit for the mechanism implemented by this class goes to
 * <A HREF="https://www.mathworks.com/matlabcentral/fileexchange/38964">
 * Oliver Woodford's Matlab File Exchange entry</A>. The main change in mexObjectHandle
 * is the C++ object is stored within mexObjectHandle rather than its pointer.
 */
template <class wrappedClass>
class mexObjectHandle
{
public:
  /**
   *\brief Instantiate the wrapped class and return its wrapper mxArray
   *
   * Create() function accepts variable arguments, which are then passed 
   * directly to the constructor of the target class, instantiating the
   * object wrapped by the mxArray.
   * 
   * The created mxArray encompasses a scalar uint64 value, which is a cast
   * of the pointer to a mexObjectHandle class instance. Before the returned
   * mxArray is destroyed (either in MATLAB or in C++), \ref destroy() or 
   * \ref _destroy() must be called to delete the pointed mexObjectHandle object.
   * 
   * \param[in] args Template parameter pack of arguments to be passed on to 
   *                 the constructor of the wrapped class.
   * \returns mxArray containing the pointer to the mexObjectHandle which
   *          wraps the instantiated wrapped class.
   */
  template <class... Args>
  static mxArray *create(Args... args)
  {
    // instantiate a new object (it may throw an exception if fails) and wrap it in an mxArray
    mxArray *out = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
    *((uint64_t *)mxGetData(out)) = reinterpret_cast<uint64_t>(new mexObjectHandle<wrappedClass>(args...));

    // lock MEX function only after successful object creation
    mexLock();
    
    return out;
  }

  /**
   * \brief Get wrapped class object from mxArray
   * 
   * This function returns a reference to the wrapped class object in the
   * given mxArray and returns it after a series of successful validation.
   * 
   * \param[in] in Pointer to wrapper mxArray object
   * \returns a reference to the managing mexObjectHandle object.
   * 
   * \throws mexRuntimeError if mxArray does not own a mexObjectHandle
   */
  static wrappedClass &getObject(const mxArray *in)
  {
    return mexObjectHandle<wrappedClass>::getHandle(in)->obj_m;
  }

  /**
 * \brief Destruct wrapped class instance (unsafe version)
 * 
 * Destruct mexObjectHandle pointed by the mxArray. This function
 * leaves mxArray data unchanged, so care must be taken after calling this function
 * to clear the mxArray content.
 * 
 * /param[in] in Pointer to wrapper mxArray object
 */
  static void _destroy(const mxArray *in)
  {
    delete mexObjectHandle<wrappedClass>::getHandle(in);

    // allow MATLAB to release the MEX function
    mexUnlock();
  }

  /**
   * \brief Destruct wrapped class instance
   * 
   * Destruct mexObjectHandle pointed by the mxArray and clear the content
   * of mxArray.
   * 
   * /param[in] in Pointer to wrapper mxArray object
   */
  static void destroy(mxArray *in)
  {
    // call the const mxArray version to perform the destruction of the object
    _destroy(in);

    // then also clear the mxArray to be an empty array
    const mwSize dims[2] = {0, 0};
    mxSetData(in, NULL);
    mxSetDimensions(in, dims, 2);
  }

private:
  /**
   * \brief mexObjectHandle constructor 
   * 
   * Instantiates the wrapped class with given arguments.
   * 
   * /param[in] args Variable arguments for the wrapped class construction
   */
  template <class... Args>
  mexObjectHandle(Args... args) : obj_m(args...), name_m(typeid(wrappedClass).name()) {}

  /**
   * \brief mexObjectHandle destruction
   * 
   * Clears name to make sure an attempt to access after _destroy() call yields in exception
   */
  ~mexObjectHandle() { name_m = NULL; }

  const char *name_m; // points to the class
  wrappedClass obj_m; // instance of the wrapped class

  /**
   * \brief Get mexObjectHandle from mxArray
   * 
   * This function obtains a reference to the mexObjectHandle object in the given mxArray and
   * returns it after a series of successful validation.
   * 
   * /param[in] in A pointer to an mxArray object
   * /returns a reference to the managing mexObjectHandle object.
   * 
   * /throws mexRuntimeError if mxArray does not own a mexObjectHandle
   */
  static mexObjectHandle<wrappedClass> *getHandle(const mxArray *in)
  {
    if (mxGetNumberOfElements(in) != 1 || mxGetClassID(in) != mxUINT64_CLASS || mxIsComplex(in))
      throw mexRuntimeError("invalidMexObjectHandle", "Input must be a real uint64 scalar.");
    mexObjectHandle<wrappedClass> *ptr = reinterpret_cast<mexObjectHandle<wrappedClass> *>(*((uint64_t *)mxGetData(in)));

    // first check that the retrieved pointer is valid
    if (!mexObjectHandle<wrappedClass>::isValidPointer(ptr))
      throw mexRuntimeError("invalidMexObjectHandle", "Handle is not a valid C++ pointer.");

    //  if (ptr->name_m!=typeid(wrappedClass).name()) // <-likely safe but not standard guaranteed alternative
    const char *type_name = typeid(wrappedClass).name();
    if (ptr->name_m != type_name &&
        (!mexObjectHandle<wrappedClass>::isValidPointer(ptr->name_m) || strcmp(ptr->name_m, type_name)))
      throw mexRuntimeError("invalidMexObjectHandle", "Handle is either invalid or not wrapping the intended C++ object.");

    return ptr;
  }

  template <typename T>
  static bool isValidPointer(T *lpAddress)
  {
#ifdef _MSC_VER
    // https://stackoverflow.com/a/35576777/4516027
    MEMORY_BASIC_INFORMATION mbi = {0};
    if (::VirtualQuery(lpAddress, &mbi, sizeof(mbi)))
    {
      DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
      bool b = mbi.Protect & mask;

      // check the page is not a guard page
      if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
        b = false;

      return b;
    }
    return false;
#else
    // **UNCHECKED** http://renatocunha.com/blog/2015/12/msync-pointer-validity/
    /* get the page size */
    size_t page_size = sysconf(_SC_PAGESIZE);
    /* find the address of the page that contains p */
    void *base = (void *)((((size_t)lpAddress) / page_size) * page_size);
    /* call msync, if it returns non-zero, return false */
    return msync(base, page_size, MS_ASYNC) == 0;
#endif
  }
};

/**
 * \brief Function to implement MATLAB class' C++ interface member function
 * 
 * mexObjectHandler may be a sole function call from mexFunction() to interface the MATLAB 
 * and C++ classes. The mexFunction should implement a private static member function of the 
 * wrapping MATLAB class, and the MATLAB class must have a (private) property named 
 * 'backend' which receives the C++ class object handle using the \ref mexObjectHandle mechanism.
 * 
 * The arguments are exactly the same as mexFunction() and its arguments could be directly 
 * passed down to mexObjectHandler():
 * 
 * \param[in]    nlhs Number of expected output mxArrays
 * \param[inout] plhs Array of pointers to the expected output mxArrays
 * \param[in]    nrhs Number of input mxArrays
 * \param[in]    prhs Array of pointers to the input mxArrays.
 * 
 * The mexFunction based on this function interact directly with the wrapping MATLAB class, via 
 * 4 basic MATLAB function signature:
 * 
 * * mexfcn(obj,varargin)          - Create a new C++ class instance and store it as a `backend` 
 *                                   MATLAB class property.
 * * mexfcn(obj,'delete')          - Destruct the C++ class instance.
 * * mexfcn(obj,'action',varargin) - Perform an action
 * * mexfcn('action',varargin)     - Perform a static action
 * 
 * Note that all non-static action signature receives the MATLAB object, enabling the C++ class
 * object to interact with MATLAB class object as needed.
 * 
 * The template class `mexClass` must either inherit \ref mexSetGetClass or match its public 
 * member function syntax to be compatible with this function. Three member functions it must 
 * provide must have the signatures:
 * 
 * * static std::string get_classname();
 * * bool action_handler(const mxArray *mxObj, const std::string &action, 
 *                       int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
 * * static bool static_handler(std::string action, int nlhs, mxArray *plhs[], 
 *                              int nrhs, const mxArray *prhs[]);
 * Also, its constructor must be compatible with the signature:
 * 
 *    constructor(const mxArray *mxObj, int nrhs, const mxArray *prhs[]);
 * 
 * where nrhs & prhs accounts for `varargin` inputs of the create-new mexFunction call.
 */
template <class mexClass>
void mexObjectHandler(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  std::string class_name = mexClass::get_classname(); // should match the associated matlab class

  try
  {
    if (nrhs < 1)
      throw mexRuntimeError(class_name + ":mex:invalidInput", "Needs at least one input argument.");

    if (!mxIsClass(prhs[0], class_name.c_str())) // static action
    {
      if (!mxIsChar(prhs[0]))
        throw mexRuntimeError(class_name + ":mex:static:functionUndefined", "Static action name not given.");
      try
      {
        std::string action = mexGetString(prhs[0]);
        if (!mexClass::static_handler(action, nlhs, plhs, nrhs - 1, prhs + 1))
        {
          std::string msg("Unknown static action: ");
          msg += action;
          throw mexRuntimeError(class_name + ":mex:static:unknownFunction", msg);
        }
      }
      catch (mexRuntimeError &e)
      {
        std::string id = e.id();
        if (id.empty()) // no id given
          throw mexRuntimeError(class_name + ":mex:static:executionFailed", e.what());
        else
          throw mexRuntimeError(class_name + ":mex:static:" + e.id(), e.what());
      }
      catch (std::exception &e)
      {
        throw mexRuntimeError(class_name + ":mex:static:executionFailed", e.what());
      }
    }
    else
    {
      // attempt to convert the first argument to the object
      mxArray *backend = mxGetProperty(prhs[0], 0, "backend");
      if (!backend)
        throw mexRuntimeError(class_name + ":unsupportedClass", "MATLAB class must have backend'' property.");
      if (mxIsEmpty(backend))
      {
        // no backend object, create a new
        if (nlhs > 1)
          throw mexRuntimeError(class_name + ":tooManyOutputArguments", "Only one argument is returned for object construction.");

        // set backend with the pointer to the newly created C++ object
        mxSetProperty((mxArray *)prhs[0], 0, "backend", mexObjectHandle<mexClass>::create(prhs[0], nrhs - 1, prhs + 1));
      }
      else if (nrhs < 2 || !mxIsChar(prhs[1]))
      {
        throw mexRuntimeError(class_name + ":missingAction", "Second argument (action) is not a string.");
      }
      else
      {
        // get the action name
        std::string action = mexGetString(prhs[1]);

        // get the C++ object (throws exception if invalid)
        mexClass &obj = mexObjectHandle<mexClass>::getObject(backend);

        // check for the delete action
        if (action == "delete")
        {
          mexObjectHandle<mexClass>::destroy(backend);
        }
        else // otherwise perform the object-specific (if run_action() is overloaded) action according to the given action
        {
          try
          {
            if (!obj.action_handler(prhs[0], action, nlhs, plhs, nrhs - 2, prhs + 2))
            {
              std::string msg("Unknown action: ");
              throw mexRuntimeError(class_name + ":unknownAction", msg + action);
            }
          }
          catch (mexRuntimeError &e)
          {
            std::string id = e.id();
            if (id.empty()) // no id given
              throw mexRuntimeError(class_name + ":mex:failedAction", e.what());
            else
              throw mexRuntimeError(class_name + ":mex:" + e.id(), e.what());
          }
          catch (std::exception &e)
          {
            throw mexRuntimeError(class_name + ":failedAction", e.what());
          }
        }
      }
    }
  }
  catch (mexRuntimeError &e)
  {
    std::string id_str = e.id();
    std::replace(id_str.begin(), id_str.end(), '.', ':'); // replace all 'x' to 'y'
    mexErrMsgIdAndTxt(id_str.c_str(), e.what());
  }
}

/**
 * \brief A near-compatible base class for mexObjectHandler function
 * 
 * This class could be used as the base class or as the template for a custom class
 * to be used with mexObjectHandler() function. It processes 4 basic actions: set, 
 * get, save, and load. Each of which must be implemented in the derived class.
 * 
 * MATLAB signature for these actions are:
 * 
 * * value = mexfcn(obj,'get',name)
 * * mexfcn(obj,'set',name,value)
 * * data = mexfcn(obj,'save')
 * * mexfcn(obj,'load',data)
 * 
 * Note that this class misses the necessary static functions: get_classname() and 
 * static_handler(). They must also be implemented in the derived class.
*/
class mexSetGetClass
{
public:
  /**
 * \brief  Perform one of basic class actions
 * 
 * \param[in]    mxObj  Associated MATLAB class object
 * \param[in]    action Name of the action to be performed
 * \param[in]    nlhs   Number of expected output mxArrays
 * \param[inout] plhs   Array of pointers to the expected output mxArrays
 * \param[in]    nrhs   Number of input mxArrays
 * \param[in]    prhs   Array of pointers to the input mxArrays.
 */
  virtual bool action_handler(const mxArray *mxObj, const std::string &action, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
    if (action == "set") // (H,"set","name",value)
    {
      // check for argument counts
      if (nlhs != 0 || nrhs != 2)
        throw mexRuntimeError("set:invalidArguments", "Set action takes 4 input arguments and returns none.");

      // get property name
      std::string name;
      try
      {
        name = mexGetString(prhs[0]);
      }
      catch (...)
      {
        throw mexRuntimeError("set:invalidPropName", "Set action's third argument must be a name string.");
      }

      // run the action
      set_prop(mxObj, name, prhs[1]);
    }
    else if (action == "get") // value = (H,"get","name")
    {
      // check for argument counts
      if (nlhs != 1 || nrhs != 1)
        throw mexRuntimeError("get:invalidArguments", "Get action takes 3 input arguments and returns one.");

      // get property name
      std::string name;
      try
      {
        name = mexGetString(prhs[0]);
      }
      catch (...)
      {
        throw mexRuntimeError("get:invalidPropName", "Get action's third argument must be a name string.");
      }

      // run the action
      plhs[0] = get_prop(mxObj, name);
    }
    else if (action == "save")
    {
      plhs[0] = save_prop(mxObj);
    }
    else if (action == "load")
    {
      load_prop(mxObj, prhs[0]);
    }
    else // no matching action found
    {
      return false;
    }

    return true;
  }

protected:
  /**
 * \brief  Set a property value
 * 
 * \param[in]    mxObj  Associated MATLAB class object
 * \param[in]    name   Name of the property
 * \param[in]    value  New property value
 */
  virtual void set_prop(const mxArray *mxObj, const std::string name, const mxArray *value) = 0;

  /**
 * \brief  Get a property value
 * 
 * \param[in]    mxObj  Associated MATLAB class object
 * \param[in]    name   Name of the property
 * \returns the property value
 */
  virtual mxArray *get_prop(const mxArray *mxObj, const std::string name) = 0;

  /**
 * \brief  Save C++ object's data
 * 
 * \param[in]    mxObj  Associated MATLAB class object
 * \returns mxArray containing the C++ object states
 */
  virtual mxArray *save_prop(const mxArray *mxObj) { return NULL; };

  /**
 * \brief  Load C++ object's data
 * 
 * \param[in] mxObj  Associated MATLAB class object
 * \param[in] data   Data to reestablish the C++ object states
 */
  virtual void load_prop(const mxArray *mxObj, const mxArray *data){};
};
