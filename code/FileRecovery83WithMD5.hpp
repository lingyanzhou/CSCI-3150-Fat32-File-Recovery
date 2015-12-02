#ifndef FILERECOVERY83WITHMD5_HPP
#define FILERECOVERY83WITHMD5_HPP
#include <string>
#include "Fat32Action.hpp"
using namespace std;
class FileRecovery83WithMD5 : public Fat32Action
{
private:
  string targetName;
  string md5String;
public:
  FileRecovery83WithMD5(const string &devName, const string &tname,
                        const string &md5)
  throw (FileIOError);
  ~FileRecovery83WithMD5()
  throw();
  void run()
  throw(FileIOError, Fat32ActionError);
};
#endif //FILERECOVERY83WITHMD5_HPP
