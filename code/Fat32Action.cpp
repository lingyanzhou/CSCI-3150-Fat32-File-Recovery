#include <string>
#include "Fat32DataAccess.hpp"
#include "Fat32Action.hpp"
using namespace std;
Fat32ActionError::Fat32ActionError(const string &what_arg)
  : runtime_error(what_arg) {}
Fat32Action::Fat32Action(const string &devName) throw(FileIOError)
  : fat32DA(devName) {}
Fat32Action::~Fat32Action() throw() {}
