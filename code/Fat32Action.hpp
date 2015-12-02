#ifndef FAT32ACTION_HPP
#define FAT32ACTION_HPP
#include <string>
#include <stdexcept>
#include "Fat32DataAccess.hpp"
using namespace std;
class Fat32ActionError : public runtime_error
{
public:
  explicit Fat32ActionError(const string &what_arg);
};
class Fat32Action
{
protected:
  Fat32DataAccess fat32DA;

public:
  Fat32Action(const string &devName)
  throw(FileIOError);
  virtual ~Fat32Action() throw();
  virtual void run() throw(FileIOError, Fat32ActionError) = 0;
};
#endif //FAT32ACTION_HPP
