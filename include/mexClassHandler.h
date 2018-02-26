/** \file mexClassHandler.h
 * C++ header file containing classes and function necessary to wrap C++ class with MATLAB class
 */

#pragma once

#include "mexGetString.h"    // to convert char mexArray to std::string
#include "mexRuntimeError.h" // for mexRuntimeError runtime exception class

#include <mex.h>
#include <stdint.h>
#include <typeinfo>

/**
 * \brief Underlying wrapper class to wrap C++ object by an mxArray object
 * 
 * mexClassHandle is a class template to wrap a C++ object so that the object can
 * be preserved over multiple mexFunction calls while allowing the creation of multiple
 * objects unlike using static variables. 
 * 
 * The original credit for the mechanism implemented by this class goes to
 * <A HREF="https://www.mathworks.com/matlabcentral/fileexchange/38964">
 * Oliver Woodford's Matlab File Exchange entry</A>. The main change in mexClassHandle
 * is the C++ object is stored within mexClassHandle rather than its pointer.
 */
template <class wrappedClass>
class mexClassHandle
{
public:
  /**
   *\brief Access the wrapped C++ object
   *
   * Returns a reference to the managed object or nullptr if no object is owned. 
   * 
   *\returns a reference to the managed object or nullptr if no object is owned. 
   */
  wrappedClass &get() { return obj_m; }

  /**
   *\brief Instantiate the wrapped class and return its wrapper mxArray
   *
   * Create() function accepts variable arguments, which are then passed 
   * directly to the constructor of the target class, instantiating the
   * object wrapped by the mxArray.
   * 
   * The created mxArray encompasses a scalar uint64 value, which is a cast
   * of the pointer to a mexClassHandle class instance. Before the returned
   * mxArray is destroyed (either in MATLAB or in C++), \ref destroy() or 
   * \ref _destroy() must be called to delete the pointed mexClassHandle object.
   * 
   * \param[in] args Template parameter pack of arguments to be passed on to 
   *                 the constructor of the wrapped class.
   * \returns mxArray containing the pointer to the mexClassHandle which
   *          wraps the instantiated wrapped class.
   */
  template <class... Args>
  static mxArray *create(Args... args)
  {
    mexLock();
    mxArray *out = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
    *((uint64_t *)mxGetData(out)) = reinterpret_cast<uint64_t>(new mexClassHandle<wrappedClass>(args...));
    return out;
  }

  /**
   *\brief Copy-construct the wrapped class instace and return its wrapper mxArray
   *
   * This create() function copy constructs the wrapped class instance, and creates 
   * returns the wrapping mxArray object.
   * 
   * The created mxArray encompasses a scalar uint64 value, which is a cast
   * of the pointer to a mexClassHandle class instance. Before the returned
   * mxArray is destroyed (either in MATLAB or in C++), \ref destroy() or 
   * \ref _destroy() must be called to delete the pointed mexClassHandle object.
   * 
   * \param[in] src Source class object.
   * \returns mxArray containing the pointer to the mexClassHandle which
   *          wraps the copied wrapped class instance.
   */
  static mxArray *create(const wrappedClass &src)
  {
    mexLock();
    mxArray *out = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
    *((uint64_t *)mxGetData(out)) = reinterpret_cast<uint64_t>(new mexClassHandle<wrappedClass>(src));
    return out;
  }

  /**
   *\brief Move-construct the wrapped class instace and return its wrapper mxArray
   *
   * This create() function move constructs the wrapped class instance, and creates 
   * returns the wrapping mxArray object.
   * 
   * The created mxArray encompasses a scalar uint64 value, which is a cast
   * of the pointer to a mexClassHandle class instance. Before the returned
   * mxArray is destroyed (either in MATLAB or in C++), \ref destroy() or 
   * \ref _destroy() must be called to delete the pointed mexClassHandle object.
   * 
   * \param[in] src Source class object.
   * \returns mxArray containing the pointer to the mexClassHandle which
   *          wraps the copied wrapped class instance.
   */
  static mxArray *create(wrappedClass &&src)
  {
    mexLock();
    mxArray *out = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
    *((uint64_t *)mxGetData(out)) = reinterpret_cast<uint64_t>(new mexClassHandle<wrappedClass>(src));
    return out;
  }

  /**
   * \brief Get wrapped class object from mxArray
   * 
   * This function returns a reference to the wrapped class object in the
   * given mxArray and returns it after a series of successful validation.
   * 
   * \param[in] in Pointer to wrapper mxArray object
   * \returns a reference to the managing mexClassHandle object.
   * 
   * \throws mexRuntimeError if mxArray does not own a mexClassHandle
   */
  static wrappedClass &getObject(const mxArray *in)
  {
    return mexClassHandle<wrappedClass>::getHandle(in)->obj_m;
  }

 /**
 * \brief Destruct wrapped class instance (unsafe version)
 * 
 * Destruct mexClassHandle pointed by the mxArray. This function
 * leaves mxArray data unchanged, so care must be taken after calling this function
 * to clear the mxArray content.
 * 
 * /param[in] in Pointer to wrapper mxArray object
 */
  static void _destroy(const mxArray *in)
  {
    delete mexClassHandle<wrappedClass>::getHandle(in);

    // allow MATLAB to release the MEX function
    mexUnlock();
  }

  /**
   * \brief Destruct wrapped class instance
   * 
   * Destruct mexClassHandle pointed by the mxArray and clear the content
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
   * \brief mexClassHandle constructor 
   * 
   * Instantiates the wrapped class with given arguments.
   * 
   * /param[in] args Variable arguments for the wrapped class construction
   */
  template <class... Args>
  mexClassHandle(Args... args) : obj_m(args...), name_m(typeid(wrappedClass).name()) {}

  /**
   * \brief mexClassHandle construction, copy-constructing wrapped class
   * 
   * Instantiates the wrapped class by copy construction.
   * 
   * /param[in] src Source wrapped class object
   */
  mexClassHandle(const wrappedClass *src) : obj_m(src), name_m(typeid(wrappedClass).name()) {}

  /**
   * \brief mexClassHandle construction, move-constructing wrapped class
   * 
   * Instantiates the wrapped class by move construction.
   * 
   * /param[in] src Source wrapped class object
   */
    mexClassHandle(wrappedClass &&src) : obj_m(src), name_m(typeid(wrappedClass).name()) {}

  /**
   * \brief mexClassHandle destruction
   * 
   * Clears name to make sure an attempt to access after _destroy() call yields in exception
   */
  ~mexClassHandle() { name_m = NULL; }

  const char* name_m; // points to the class
  wrappedClass obj_m; // instance of the wrapped class

  /**
   * \brief Get mexClassHandle from mxArray
   * 
   * This function obtains a reference to the mexClassHandle object in the given mxArray and
   * returns it after a series of successful validation.
   * 
   * /param[in] in A pointer to an mxArray object
   * /returns a reference to the managing mexClassHandle object.
   * 
   * /throws mexRuntimeError if mxArray does not own a mexClassHandle
   */
  static mexClassHandle<wrappedClass> *getHandle(const mxArray *in)
  {
    if (mxGetNumberOfElements(in) != 1 || mxGetClassID(in) != mxUINT64_CLASS || mxIsComplex(in))
      throw mexRuntimeError("invalidMexObjectHandle", "Input must be a real uint64 scalar.");
    mexClassHandle<wrappedClass> *ptr = reinterpret_cast<mexClassHandle<wrappedClass> *>(*((uint64_t *)mxGetData(in)));

    //  if (ptr->name_m!=typeid(wrappedClass).name()) // <-likely safe but not standard guaranteed alternative
    if (!ptr->name_m || strcmp(ptr->name_m, typeid(wrappedClass).name()))
      throw mexRuntimeError("invalidMexObjectHandle", "Handle not valid.");

    return ptr;
  }
};

/**
 * \brief Function to implement MATLAB class' C++ interface member function
 * 
 * mexClassHandler may be a sole function call from mexFunction() to interface the MATLAB 
 * and C++ classes. The mexFunction should implement a private static member function of the 
 * wrapping MATLAB class, and the MATLAB class must have a (private) property named 
 * 'backend' which receives the C++ class object handle using the \ref mexClassHandle mechanism.
 * 
 * The arguments are exactly the same as mexFunction() and its arguments could be directly 
 * passed down to mexClassHandler():
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
 *    constructor(int nrhs, const mxArray *prhs[]);
 * 
 * where nrhs & prhs accounts for `varargin` inputs of the create-new mexFunction call.
 */
template <class mexClass>
void mexClassHandler(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
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
        mxSetProperty((mxArray *)prhs[0], 0, "backend", mexClassHandle<mexClass>::create(nrhs - 1, prhs + 1));
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
        mexClass &obj = mexClassHandle<mexClass>::getObject(backend);

        // check for the delete action
        if (action == "delete")
        {
          mexClassHandle<mexClass>::destroy(backend);
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
 * \brief A near-compatible base class for mexClassHandler function
 * 
 * This class could be used as the base class or as the template for a custom class
 * to be used with mexClassHandler() function. It processes 4 basic actions: set, 
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
