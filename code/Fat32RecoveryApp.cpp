#include <string>
#include <stdexcept>
#include <iostream>
#include "Fat32Action.hpp"
#include "PrintBootSectorInfo.hpp"
#include "ListAllDirectoryEntry.hpp"
#include "FileRecovery83.hpp"
#include "FileRecovery83WithMD5.hpp"
#include "FileRecoveryLong.hpp"
#include "Fat32RecoveryApp.hpp"

using namespace std;

InvalidArgumentError::
InvalidArgumentError(const string &what_arg)
  : runtime_error("Invalid Argument: " + what_arg)
{
}
Fat32RecoveryApp::
Fat32RecoveryApp(char *name) throw()
  : appName(name), action(NULL)
{
}
Fat32RecoveryApp::
~Fat32RecoveryApp() throw()
{
  if (NULL != action) {
    delete action;
  }
}
void Fat32RecoveryApp::
parseArgument(const int argc, char **argv)
throw (InvalidArgumentError, FileIOError)
{
  string deviceName;
  string targetName;
  string md5String;
  bool has_d = false;
  bool has_i = false;
  bool has_l = false;
  bool has_r = false;
  bool has_m = false;
  bool has_R = false;

  for (int i = 1; i < argc; i++) {
    string argcur = argv[i];

    if (argcur == "-d") {
      if ( !has_d && i + 1 < argc) {
        i++;
        deviceName = argv[i];
        has_d = true;
      } else {
        printUsage();
        throw InvalidArgumentError("around -d");
      }
    } else if (argcur == "-i") {
      if (!has_i && !has_l && !has_r && !has_m && !has_R) {
        has_i = true;
      } else {
        printUsage();
        throw InvalidArgumentError("around -i");
      }
    } else if (argcur == "-l") {
      if (!has_l && !has_i && !has_r && !has_m && !has_R) {
        has_l = true;
      } else {
        printUsage();
        throw InvalidArgumentError("around -l");
      }
    } else if (argcur == "-r") {
      if ( !has_r && !has_l && !has_i && !has_R && i + 1 < argc) {
        i++;
        targetName = argv[i];
        has_r = true;
      } else {
        printUsage();
        throw InvalidArgumentError("around -r");
      }
    } else if (argcur == "-m") {
      if ( !has_m && !has_l && !has_i && !has_R && i + 1 < argc) {
        i++;
        md5String = argv[i];
        has_m = true;
      } else {
        printUsage();
        throw InvalidArgumentError("around -m");
      }
    } else if (argcur == "-R") {
      if ( !has_R && !has_i && !has_r && !has_l && !has_m && i + 1 < argc) {
        i++;
        targetName = argv[i];
        has_R = true;
      } else {
        printUsage();
        throw InvalidArgumentError("-R");
      }
    } else {
      printUsage();
      throw InvalidArgumentError("Invalid option: "+argcur);
    }
  }

  if ( !has_d || !( has_i || has_l || has_r || has_R) ) {
    printUsage();
    throw InvalidArgumentError("Device or action not specified");
  }

#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "-d :" << has_d
      << " | deviceName: " << deviceName
      << " | -i: " << has_i
      << " | -l: " << has_l
      << " | -m: " << has_m
      << " | md5: " << md5String
      << " | -r: " << has_r
      << " | -R: " << has_R
      << " | targetName: " << targetName << endl;
  cout << "\x1b[0m";
#endif //DEBUG

  if (has_i) {
    action = new PrintBootSectorInfo(deviceName);
  } else if (has_l) {
    action = new ListAllDirectoryEntry(deviceName);
  } else if (has_r && !has_m) {
    action = new FileRecovery83(deviceName, targetName);
  } else if (has_r && has_m) {
    action = new FileRecovery83WithMD5(deviceName, targetName, md5String);
  } else if (has_R) {
    action = new FileRecoveryLong(deviceName, targetName);
  }
}
void Fat32RecoveryApp::
run() throw(FileIOError)
{
  try {
    action->run();
#ifdef DEBUG
    cout << "\x1b[7m";
    cout << "Done" << endl;
    cout << "\x1b[0m";
#endif //DEBUG
  } catch (Fat32ActionError &e) {
    cout << e.what() << endl;
  }
}
void Fat32RecoveryApp::printUsage() throw() {
  try {
    cout << "Usage: " << appName << " -d [device filename] [other arguments]"
         << endl;
    cout << "-i                    Print boot sector information" << endl;
    cout << "-l                    List all the directory entries" << endl;
    cout << "-r filename [-m md5]  File recovery with 8.3 filename" << endl;
    cout << "-R filename           File recovery with long filename" << endl;
  }
  catch (...) {
  }
}

