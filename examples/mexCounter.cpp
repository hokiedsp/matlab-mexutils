#include <mex.h>
#include <mexObjectHandler.h>
#include <mexGetString.h>

/**
 * \brief   mexCounter: demonstration MEX function for mexObjectHandle
 * 
 * This mexFuction acts as a simple counter and shall be called from MATLAB
 * with 3 signatures:
 * 
 * * cnt = mexCounter()       - Create new object and return an mxArray pointer to its counter state
 * * mexCounter(cnt,'delete') - Destroys the existing counter state pointed by cnt
 * * val = mexCounter(cnt)    - Increments the counter and returns its pre-increment value
 * 
 * See mexCounter_demo.m for the Matlab demo using this MEX function.
 */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  if (nrhs == 0) // return new object
  {
    mexPrintf("[mexCounter] Creating a new mexObjectHandle<int>\n");
    plhs[0] = mexObjectHandle<int>::create(0);
    return;
  }

  if (nrhs > 1) // given command: 'delete'
  {
    std::string cmd = mexGetString(prhs[1]);
    if (cmd == "delete")
    {
      mexObjectHandle<int>::_destroy(prhs[0]);
      mexPrintf("[mexCounter] The mexObjectHandle<int> has been destroyed.\n");
    }
    else
      mexErrMsgTxt("Unknown mexObjectHandle command.");
  }
  else // no command: set return value and increment the counter
  {
    int &counter = mexObjectHandle<int>::getObject(prhs[0]);
    plhs[0] = mxCreateDoubleScalar((double)counter);
    ++counter;
  }
}
