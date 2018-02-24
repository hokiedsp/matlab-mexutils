#pragma once

#include "mexGetString.h"    // to convert char mexArray to std::string
#include "mexRuntimeError.h" // for mexRuntimeError runtime exception class

#include <mex.h>
#include <stdint.h>

#define CLASS_HANDLE_SIGNATURE 0xFF00F0A5
template <class base>
class mexClassHandle
{
public:
  /**
   *\brief Access the managing C++ object pointer
   *
   * Returns a pointer to the managed object or nullptr if no object is owned. 
   * 
   *\returns a pointer to the managed object or nullptr if no object is owned. 
   */
  base *get() { return ptr_m; }

  /**
   *\brief Create new mexClassHandle
   *
   * All mexClassHandle objects must be created with this function. The
   * function returns a pointer to a new mxArray object, which contains
   * the new mexClassHandle object. To access the mexClassHandle object,
   * use \ref getHandle(). To access the managed C++ object, use \ref
   * getObject().
   * 
   * Calling this function locks the MEX function (i.e., shared library) 
   * in memory until \ref destroy() is called on this object. Multiple
   * mexClassHandles may be employed in an MEX function as the lock is
   * counted; the MEX function won't be freed until all locks are released.
   * 
   * \param[in] ptr A pointer to an unmanaged C++ object
   * \returns mxArray containing the pointer to the mexClassHandle which
   *          manages \ref ptr.
   */
  static mxArray *create(base *ptr)
  {
    mexLock();
    mxArray *out = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
    *((uint64_t *)mxGetData(out)) = reinterpret_cast<uint64_t>(new mexClassHandle<base>(ptr));
    return out;
  }

  /**
   * \brief Get mexClassHandle from mxArray
   * 
   * This function obtains a reference to the mexClassHandle object in the given mxArray and
   * returns it after a series of successful validation.
   * 
   * /param[in] in A pointer to an mxArray object
   * /returns a reference to the managing mexClassHandle object.
   */
  static mexClassHandle<base> &getHandle(const mxArray *in)
  {
    if (mxGetNumberOfElements(in) != 1 || mxGetClassID(in) != mxUINT64_CLASS || mxIsComplex(in))
      throw mexRuntimeError("invalidMexObjectHandle", "Input must be a real uint64 scalar.");
    mexClassHandle<base> &ptr = *reinterpret_cast<mexClassHandle<base> *>(*((uint64_t *)mxGetData(in)));
    if (!ptr.isValid())
      throw mexRuntimeError("invalidMexObjectHandle", "Handle not valid.");
    return ptr;
  }

  /**
   * \brief Get managed class object from mxArray
   * 
   * This function directly obtains a pointer to the managed class object in the
   * given mxArray and returns it after a series of successful validation.
   * 
   * /param[in] in A pointer to an mxArray object
   * /returns a pointer to the managing mexClassHandle object.
   */
  static base *getObject(const mxArray *in)
  {
    return mexClassHandle<base>::getHandle(in).ptr_m;
  }

  static void destroy(const mxArray *in, bool delete_obj = true)
  {
    mexClassHandle<base> &ref = mexClassHandle<base>::getHandle(in);
    if (!delete_obj)
      ref.ptr_m = NULL;

    // destruct the underlying C++ class instance
    delete &ref;

    // allow MATLAB to release the MEX function
    mexUnlock();
  }

private:
  mexClassHandle(base *ptr) : ptr_m(ptr), name_m(typeid(base).name()) { signature_m = CLASS_HANDLE_SIGNATURE; }
  ~mexClassHandle()
  {
    signature_m = 0;
    if (ptr_m) delete ptr_m;
  }
  bool isValid() { return ((signature_m == CLASS_HANDLE_SIGNATURE) && !strcmp(name_m.c_str(), typeid(base).name())); }

  uint32_t signature_m;
  std::string name_m;
  base *ptr_m;
};

/// mexClassHandler
/// Function to handle MATLAB class member mex-function calls. The associated MATLAB class must have a (private)
/// property named 'backend' which stores the C++ class object handle (must inherit mexClassHandle).
///
/// In Matlab, the associated class object must have a private property 'backend' to hold a mexClassHandle object as well as
/// a private STATIC method (method's name does not matter, we assume 'mexfcn' in this document).
///
/// mexClass C++ class must provide get_classname() static member function which returns the associated Matlab class name
/// for verification purpose
///
/// There are three basic types of calls from MATLAB (assuming the MATLAB class name to be MATCLASS and its object name to be OBJ):
/// * MATCLASS.mexfcn(OBJ,varargin)
///   This calls the backend class constructor if OBJ.backend property is not populated and populates it with the C++ object handle.
///   the second argument maybe omitted.
/// * varargout = MATCLASS.mexfcn(OBJ,'action',varargin)
///   This implements dynamic class action. OBJ is expected to be a scalar entity (if not only the first element is used)
/// * varargout = MATCLASS.mexfcn('action',varargin)
///   This implements static class action.
///
/// There are three preset dynamic actions:
/// * action='delete' : destructor
/// * action='get'    : property get : implemented by mexClass::get_prop
/// * action='set'    : property set : implemented by mexClass::set_prop
/// * custom actions  :                implemented by mexClass::action_handler
template <class mexClass>
void mexClassHandler(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  std::string class_name = mexClass::get_classname(); // associated matlab class

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
        mexClass *ptr(NULL);
        try
        {
          ptr = new mexClass(nrhs - 1, prhs + 1);
          if (ptr == NULL)
            throw mexRuntimeError(class_name + ":mex:constructorFail", "Constructor failed silently.");
        }
        catch (mexRuntimeError &e)
        {
          std::string id = e.id();
          if (id.empty()) // no id given
            throw mexRuntimeError(class_name + ":mex:constructorFail", e.what());
          else
            throw mexRuntimeError(class_name + ":mex:" + e.id(), e.what());
        }
        catch (std::exception &e)
        {
          throw mexRuntimeError(class_name + ":mex:constructorFail", e.what());
        }

        // set backend with the pointer to the newly created C++ object
        mxSetProperty((mxArray *)prhs[0], 0, "backend", mexClassHandle<mexClass>::create(ptr));
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
        mexClass *obj = mexClassHandle<mexClass>::getObject(backend);

        // check for the delete action
        if (action == "delete")
        {
          mexClassHandle<mexClass>::destroy(backend);
        }
        else // otherwise perform the object-specific (if run_action() is overloaded) action according to the given action
        {
          try
          {
            if (!obj->action_handler(prhs[0], action, nlhs, plhs, nrhs - 2, prhs + 2))
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

// Base class for an internal MEX class interfaced a MATLAB class object using mexClassHandle class
class mexFunctionClass
{
public:
  // static std::string get_classname() { return "mexClassGeneric"; };

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

  static bool static_handler(std::string action, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
    return false;
  }

protected:
  virtual void set_prop(const mxArray *mxObj, const std::string name, const mxArray *value) = 0;
  virtual mxArray *get_prop(const mxArray *mxObj, const std::string name) = 0;

  virtual mxArray *save_prop(const mxArray *mxObj) { return NULL; };
  virtual void load_prop(const mxArray *mxObj, const mxArray *value){};
};
