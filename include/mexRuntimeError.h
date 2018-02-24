#pragma once

#include <string>
#include <stdexcept>

/**
 * \brief mexErrMsgAndTxt friendly runtime exception class
 */
class mexRuntimeError : public std::runtime_error
{
public:
  mexRuntimeError(char const *const message) throw() : std::runtime_error(message)
  {
  }
  mexRuntimeError(char const *ident, char const *const message) throw() : std::runtime_error(message), id_str(ident)
  {
  }
  mexRuntimeError(std::string &message) throw() : std::runtime_error(message.c_str())
  {
  }
  mexRuntimeError(std::string &ident, std::string &message) throw() : std::runtime_error(message.c_str()), id_str(ident)
  {
  }
  mexRuntimeError(std::string &ident, char const *const message) throw() : std::runtime_error(message), id_str(ident)
  {
  }
  virtual char const *id() const throw() { return id_str.c_str(); }

private:
  std::string id_str;
};
