#ifndef FILERECOVERYLONG_HPP
#define FILERECOVERYLONG_HPP

#include <string>
#include "Fat32Action.hpp"
using namespace std;
class FileRecoveryLong : public Fat32Action
{
private:
  string targetName;
public:
  FileRecoveryLong(const string &devName, const string &tname)
  throw (FileIOError);
  ~FileRecoveryLong()
  throw();
  void run()
  throw(FileIOError, Fat32ActionError);
};
#endif //FILERECOVERYLONG_HPP
