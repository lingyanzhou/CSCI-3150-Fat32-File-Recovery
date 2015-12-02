#include <string>
#include <iostream>
#include "Fat32Action.hpp"
#include "Fat32DataAccess.hpp"
#include "PrintBootSectorInfo.hpp"
using namespace std;
PrintBootSectorInfo::
PrintBootSectorInfo(const string &devName)  throw (FileIOError)
  : Fat32Action(devName)
{
}
PrintBootSectorInfo::
~PrintBootSectorInfo() throw()
{
}
void PrintBootSectorInfo::
run()  throw(FileIOError, Fat32ActionError)
{
  cout << "Number of FATs = " << fat32DA.getNumFATs() << endl;
  cout << "Number of bytes per sector = " << fat32DA.getBytsPerSec() << endl;
  cout << "Number of sectors per cluster = " << fat32DA.getSecPerClus() << endl;
  cout << "Number of reserved sectors = " << fat32DA.getRsvdSecCnt() << endl;
  cout << "Number of allocated clusters = " << fat32DA.getAllocClusCnt() << endl;
  cout << "Number of free clusters = " << fat32DA.getFreeClusCnt() << endl;
}
