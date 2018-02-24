#pragma once

#include "mexRuntimeError.h"

#include <mex.h>
#include <string>

std::string mexGetString(const mxArray *array)
{
  // ideally use std::codecvt but VSC++ does not support this particular template specialization as of 2017
  // mxChar *str_utf16 = mxGetChars(array);
  // if (str_utf16==NULL)
  //   throw 0;
  // std::string str = std::wstring_convert<std::codecvt_utf8_utf16<mxChar>, mxChar>{}.to_bytes(str_utf16);

  // convert a scalar cell-string 
  if (mxIsCell(array) && mxIsScalar(array))
    return mexGetString(mxGetCell(array, 0));

  mwSize len = mxGetNumberOfElements(array);
  std::string str;
  str.resize(len + 1);
  if (mxGetString(array, &str.front(), str.size()) != 0)
    throw mexRuntimeError("notString", "Failed to convert MATLAB string.");
  str.resize(len); // remove the trailing NULL character
  return str;
}
