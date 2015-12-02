#include <string>
#include <list>
#include <utility>
#include <iostream>
#include "Fat32Action.hpp"
#include "Fat32DataAccess.hpp"
#include "FileRecoveryLong.hpp"
using namespace std;
FileRecoveryLong::FileRecoveryLong(const string &devName,
                                   const string &tname) throw(FileIOError)
    : Fat32Action(devName), targetName(tname) {}
FileRecoveryLong::~FileRecoveryLong() throw() {}
void FileRecoveryLong::run() throw(FileIOError, Fat32ActionError) {
  if (targetName.length() == 0) {
    throw Fat32ActionError(targetName + ": error - file not found");
  }

  list<FileHandler> matchedList;
  FileHandler dirHandler = fat32DA.getRootHandler();

  try {
    while (true) {
      FileHandler fh = fat32DA.getNextFileHandlerFromDir(dirHandler);

      if (fh.isDeleted()) {
#ifdef DEBUG
        cout << "\x1b[7m";
        cout << "Found Deleted: " << fh.toString() << endl;
        cout << "\x1b[0m";
#endif //DEBUG

        if (0 == targetName.compare(fh.getLongName())) {
          matchedList.push_back(fh);
#ifdef DEBUG
          cout << "\x1b[7m";
          cout << "Match. Queued" << endl;
          cout << "\x1b[0m";
#endif //DEBUG
        } else {
#ifdef DEBUG
          cout << "\x1b[7m";
          cout << "Not match. Skipped" << endl;
          cout << "\x1b[0m";
#endif //DEBUG
        }
      } else {
#ifdef DEBUG
        cout << "\x1b[7m";
        cout << "Not Deleted. Ignored: " << fh.toString() << endl;
        cout << "\x1b[0m";
#endif //DEBUG
      }
    }
  }
  catch (NoMoreData & e) {
  }

  int matchNum = matchedList.size();

  if (0 == matchNum) {
#ifdef DEBUG
    cout << "\x1b[7m";
    cout << "No match found" << endl;
    cout << "\x1b[0m";
#endif //DEBUG
    throw Fat32ActionError(targetName + ": error - file not found");
  } else if (1 == matchNum) {
#ifdef DEBUG
    cout << "\x1b[7m";
    cout << "1 match found" << endl;
    cout << "\x1b[0m";
#endif //DEBUG
    FileHandler &fh = matchedList.front();
#ifdef DEBUG
    cout << "\x1b[7m";
    cout << "Processing: " << fh.toString() << endl;
    cout << "\x1b[0m";
#endif //DEBUG

    try {
      fat32DA.recover(fh, targetName[0], true);
      cout << targetName << ": recovered" << endl;
    }
    catch (ClusterOccupied & e) {
      throw Fat32ActionError(targetName + ": error - fail to recover");
    }
  } else {
#ifdef DEBUG
    cout << "\x1b[7m";
    cout << matchNum << " match found" << endl;
    cout << "\x1b[0m";
#endif //DEBUG
    throw Fat32ActionError(targetName + ": error - ambiguous");
  }
}
