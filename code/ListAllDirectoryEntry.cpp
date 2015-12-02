#include <string>
#include <iostream>
#include "Fat32Action.hpp"
#include "Fat32DataAccess.hpp"
#include "ListAllDirectoryEntry.hpp"
using namespace std;
ListAllDirectoryEntry::
ListAllDirectoryEntry(const string &devName)
throw(FileIOError)
  : Fat32Action(devName)
{
}
ListAllDirectoryEntry::
~ListAllDirectoryEntry()
throw()
{
}
void ListAllDirectoryEntry::
run()
throw(FileIOError, Fat32ActionError)
{
  FileHandler dirHandler = fat32DA.getRootHandler();
  unsigned int i = 0;

  try {
    while (true) {
      FileHandler fh = fat32DA.getNextFileHandlerFromDir(dirHandler);

      if (fh.isDeleted()) {
#ifdef DEBUG
        cout << "\x1b[7m";
        cout << i << ", " << fh.toString() << endl;
        cout << "\x1b[0m";
#endif //DEBUG
      } else {
        i++;
        cout << i << ", " << fh.toString() << endl;
      }
    }
  } catch (NoMoreData &e) {
  }
}
