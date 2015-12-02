#include <iostream>
#include "Fat32RecoveryApp.hpp"
#include "Fat32DataAccess.hpp"
using namespace std;
int main(int argc, char **argv)
{
  Fat32RecoveryApp recoveryApp(argv[0]);

  try {
    recoveryApp.parseArgument(argc, argv);
    recoveryApp.run();
  } catch (InvalidArgumentError &e) {
#ifdef DEBUG
    cout << "\x1b[7m";
    cout << e.what() << endl;
    cout << "\x1b[0m";
#endif //DEBUG
  } catch (FileIOError &e) {
#ifdef DEBUG
    cout << "\x1b[7m";
    cout << e.what() << endl;
    cout << "\x1b[0m";
#endif //DEBUG
  }
}
