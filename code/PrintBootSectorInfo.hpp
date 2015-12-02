#ifndef PRINTBOOTSECTORINFO_HPP
#define PRINTBOOTSECTORINFO_HPP
#include <string>
#include "Fat32Action.hpp"
using namespace std;
class PrintBootSectorInfo : public Fat32Action
{
public:
  PrintBootSectorInfo(const string &devName) throw (FileIOError);
  ~PrintBootSectorInfo() throw();
  void run()  throw(FileIOError, Fat32ActionError);
};
#endif
