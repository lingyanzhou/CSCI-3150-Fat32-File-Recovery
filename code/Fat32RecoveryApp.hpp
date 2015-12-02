#ifndef FAT32RECOVERYAPP_HPP
#define FAT32RECOVERYAPP_HPP
#include <string>
#include <stdexcept>
#include "Fat32Action.hpp"
using namespace std;

class InvalidArgumentError : public runtime_error
{
public:
  explicit InvalidArgumentError (const string &what_arg);
};

class Fat32RecoveryApp
{
private:
  string appName;
  Fat32Action *action;
public:
  Fat32RecoveryApp(char *name) throw();
  ~Fat32RecoveryApp() throw();
  void parseArgument(const int argc, char **argv)
  throw (InvalidArgumentError, FileIOError);
  void run() throw(FileIOError);
  void printUsage() throw();
};
#endif //FAT32RECOVERYAPP_HPP
