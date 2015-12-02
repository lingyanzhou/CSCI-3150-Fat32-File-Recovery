#include <string>
#include <list>
#include <utility>
#include <iostream>
#include <memory>
#include <openssl/md5.h>
#include "Fat32Action.hpp"
#include "Fat32DataAccess.hpp"
#include "FileRecovery83WithMD5.hpp"
using namespace std;
FileRecovery83WithMD5::FileRecovery83WithMD5(
    const string &devName, const string &tname,
    const string &md5) throw(FileIOError)
    : Fat32Action(devName), targetName(tname), md5String(md5) {}
FileRecovery83WithMD5::~FileRecovery83WithMD5() throw() {}
void FileRecovery83WithMD5::run() throw(FileIOError, Fat32ActionError) {
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

        if (0 == targetName.compare(1, string::npos, fh.getShortName(), 1,
                                    string::npos)) {
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
  } else {
#ifdef DEBUG
    cout << "\x1b[7m";
    cout << matchNum << " initial match found" << endl;
    cout << "\x1b[0m";
#endif //DEBUG

    for (list<FileHandler>::iterator it = matchedList.begin();
         it != matchedList.end(); ++it) {
      try {
        FileHandler &fh = *it;
#ifdef DEBUG
        cout << "\x1b[7m";
        cout << "Processing: " << fh.toString() << endl;
        cout << "\x1b[0m";
#endif //DEBUG
        unique_ptr<char[]> buf(new char[fh.getSize()]);
        unsigned char digest[MD5_DIGEST_LENGTH];

        while (true) {
          ssize_t readStatus;
          off_t offset = 0;
          readStatus = fat32DA.fs32read(fh, buf.get() + offset, fh.getSize());

          if (0 == readStatus) {
            break;
          } else {
            offset += readStatus;
          }
        }

#ifdef DEBUG
        cout << "\x1b[7m";
        cout << hex;
        cout << "First few bytes:" << endl;

        for (uint32_t i = 0; i < fh.getSize() && i < 10; i++) {
          cout << " 0x" << (int) buf[i];
        }

        cout << endl;
        cout << "Last few bytes:" << endl;
        int j = fh.getSize() > 10 ? fh.getSize() - 10 : 0;
        cout << j << endl;

        for (uint32_t i = j; i < fh.getSize(); i++) {
          cout << " 0x" << (int) buf[i];
        }

        cout << endl;
        cout << dec;
        cout << "\x1b[0m";
#endif //DEBUG
#ifdef DEBUG
        cout << "\x1b[7m";
        cout << "Calculating MD5..." << endl;
        cout << "\x1b[0m";
#endif //DEBUG
        MD5((unsigned char *)buf.get(), fh.getSize(), (unsigned char *)&digest);
        char md5cstr[33] = { 0 };

        for (int i = 0; i < 16; i++) {
          sprintf(&md5cstr[i * 2], "%02x", (unsigned int) digest[i]);
        }

#ifdef DEBUG
        cout << "\x1b[7m";
        cout << "MD5: " << md5cstr << endl;
        cout << "\x1b[0m";
#endif //DEBUG
        string md5StringCalc = md5cstr;

        if (0 == md5String.compare(md5StringCalc)) {
#ifdef DEBUG
          cout << "\x1b[7m";
          cout << "Md5 Matched. Recovering " << targetName << endl;
          cout << "\x1b[0m";
#endif //DEBUG
          fat32DA.recover(fh, targetName[0], false);
          cout << targetName << ": recovered with MD5" << endl;
          return;
        } else {
#ifdef DEBUG
          cout << "\x1b[7m";
          cout << "Md5 not match. Skipped" << endl;
          cout << "\x1b[0m";
#endif //DEBUG
        }
      }
      catch (ClusterOccupied & e) {
#ifdef DEBUG
        cout << "\x1b[7m";
        cout << "Cluster occupied. fail to recover" << endl;
        cout << "\x1b[0m";
#endif //DEBUG
        throw Fat32ActionError(targetName + ": error - fail to recover");
      }
      catch (BrokenFATChain & e) {
#ifdef DEBUG
        cout << "\x1b[7m";
        cout << "File spanning across multiple clusters. Unable to recover"
             << endl;
        cout << "\x1b[0m";
#endif //DEBUG
      }
    }

    throw Fat32ActionError(targetName + ": error - file not found");
  }
}
