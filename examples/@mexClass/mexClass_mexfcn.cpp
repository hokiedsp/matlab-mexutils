#include "mex.h"
#include "mexObjectHandler.h"

#include <vector>
#include <algorithm>

class mexClass;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  mexObjectHandler<mexClass>(nlhs, plhs, nrhs, prhs);
}

// The class that we are interfacing to
class mexClass : public mexSetGetClass
{
public:
  mexClass(int nrhs, const mxArray *prhs[]) : VarA(1), VarB({1.0f, 2.0f, 3.0f}), VarC("StringVar")
  {}

  static std::string get_classname() { return "mexClass_demo"; }; // must match the Matlab classname

  static bool static_handler(std::string command, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
    if (command=="static_fcn")
      mexClass::static_fcn(nlhs,plhs,nrhs,prhs);
    else
      return false;
    return true;
  }

  virtual bool action_handler(const mxArray *mxObj, const std::string &command, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
    // try the base class action (set, get, save, load) first, returns true if action has been performed
    if (mexSetGetClass::action_handler(mxObj, command, nlhs, plhs, nrhs, prhs))
      return true;

    if (command == "train")
    {
      // validate the arguments
      if (nlhs != 0 || nrhs != 0)
        throw mexRuntimeError(get_classname() + ":train:invalidArguments", "Train command takes no additional input argument and produces no output argument.");

      // run the action
      train();
    }
    else if (command == "test")
    {
      int id;

      // validate the arguments
      if (nlhs != 0 || nrhs != 1)
        throw mexRuntimeError(get_classname() + ":test:invalidArguments", "Test command takes one additional input argument and produces no output argument.");

      try
      {
        if (!(mxIsNumeric(prhs[0]) && mxIsScalar(prhs[0])) || mxIsComplex(prhs[0]))
          throw 0;
        double val = mxGetScalar(prhs[0]);
        id = (int)val;
        if (id != val)
          throw 0;
      }
      catch (...)
      {
        throw mexRuntimeError(get_classname() + ":test:invalidArguments", "ID input must be an integer.");
      }

      // run the action
      test(id);
    }
    else
      return false;
    return true;
  }

protected:
  void set_prop(const mxArray *mxObj, const std::string name, const mxArray *value)
  {
    if (name == "VarA") // integer between -10 and 10
    {
      try
      {
        if (!(mxIsNumeric(value) && mxIsScalar(value)) || mxIsComplex(value))
          throw 0;
        double val = mxGetScalar(value);
        int temp = (int)val;
        if (temp != val || temp < -10 || temp > 10)
          throw 0;
        VarA = temp;
      }
      catch (...)
      {
        throw mexRuntimeError(get_classname() + ":invalidPropertyValue", "VarA must be a scalar integer between -10 and 10.");
      }
    }
    else if (name == "VarB")
    {
      try
      {
        if (!mxIsDouble(value) || mxIsComplex(value))
          throw 0;
        if (mxIsEmpty(value))
        {
          VarB.clear();
        }
        else
        {
          mwSize ndims = mxGetNumberOfDimensions(value);
          const mwSize *dims = mxGetDimensions(value);
          if (ndims > 2 || (dims[0] > 1 && dims[1] > 1))
            throw 0;
          mwSize dim = dims[0] == 1 ? dims[1] : dims[0];
          double *d_val = mxGetPr(value);
          VarB.resize(dim);
          std::copy_n(d_val, dim, VarB.begin());
        }
      }
      catch (...)
      {
        throw mexRuntimeError(get_classname() + ":invalidPropertyValue", "VarB must be a vector of (real) doubles.");
      }
    }
    else if (name == "VarC")
    {
      if (mxGetM(value) != 1)
        throw mexRuntimeError(get_classname() + ":invalidPropertyValue", "VarC does not support multi-row string.");
      try
      {
        VarC = mexGetString(value);
      }
      catch (...)
      {
        throw mexRuntimeError(get_classname() + ":invalidPropertyValue", "VarC must be a character string.");
      }
    }
    else
    {
      throw mexRuntimeError(get_classname() + ":invalidPropertyName", std::string("Unknown property name:") + name);
    }
  }

  mxArray *get_prop(const mxArray *mxObj, const std::string name)
  {
    mxArray *rval;
    if (name == "VarA") // integer between -10 and 10
    {
      rval = mxCreateDoubleScalar((double)VarA);
    }
    else if (name == "VarB")
    {
      rval = mxCreateDoubleMatrix(VarB.size(), 1, mxREAL);
      std::copy(VarB.begin(), VarB.end(), mxGetPr(rval));
    }
    else if (name == "VarC")
    {
      rval = mxCreateString(VarC.c_str());
    }
    else
    {
      throw mexRuntimeError(get_classname() + ":invalidPropertyName", std::string("Unknown property name:") + name);
    }
    return rval;
  }

  mxArray *save_prop(const mxArray *mxObj)
  {
    // save as MATLAB struct (not most efficient/compact, but easiest)
    const int nfields = 3;
    const char *fieldnames[3] = {"A", "B", "C"};
    mxArray *plhs = mxCreateStructMatrix(1, 1, nfields, fieldnames);
    mxSetField(plhs, 0, "A", get_prop(mxObj, "VarA"));
    mxSetField(plhs, 0, "B", get_prop(mxObj, "VarB"));
    mxSetField(plhs, 0, "C", get_prop(mxObj, "VarC"));
    return plhs;
  }

  void load_prop(const mxArray *mxObj, const mxArray *value)
  {
    // no error check as only mexClass_demo class would call save/load ops
    set_prop(mxObj,"VarA",mxGetField(value,0,"A"));
    set_prop(mxObj,"VarB",mxGetField(value,0,"B"));
    set_prop(mxObj,"VarC",mxGetField(value,0,"C"));
  }

private:
  // mexClass variables and functions
  int VarA;
  std::vector<double> VarB;
  std::string VarC;

  void train() { mexPrintf("Executing train()\n"); }
  void test(int id) { mexPrintf("Executing test(%d)\n", id); }
  static void static_fcn(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) { mexPrintf("Executing static function\n"); }
};
