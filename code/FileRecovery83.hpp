#ifndef FILERECOVERY83_HPP
#define FILERECOVERY83_HPP
#include <string>
#include "Fat32Action.hpp"
using namespace std;
class FileRecovery83 : public Fat32Action
{
private:
  string targetName;
public:
  FileRecovery83(const string &devName, const string &tname)
  throw(FileIOError);
  ~FileRecovery83()
  throw();
  void run()
  throw(FileIOError, Fat32ActionError);
};
#endif //FILERECOVERY83_HPP
