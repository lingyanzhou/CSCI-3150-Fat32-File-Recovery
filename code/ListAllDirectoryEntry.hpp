#ifndef LISTALLDIRECTORY_HPP
#define LISTALLDIRECTORY_HPP
#include <string>
#include "Fat32Action.hpp"
using namespace std;
class ListAllDirectoryEntry : public Fat32Action
{
public:
  ListAllDirectoryEntry(const string &devName)
  throw(FileIOError);
  ~ListAllDirectoryEntry()
  throw();
  void run()
  throw(FileIOError, Fat32ActionError);
};
#endif //LISTALLDIRECTORY_HPP
