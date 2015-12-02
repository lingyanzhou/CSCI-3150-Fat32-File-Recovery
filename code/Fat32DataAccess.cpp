#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <system_error>
#include <stdint.h>
#include <string.h>
#include <endian.h>
#include <list>
#include <vector>
#include <errno.h>
#include <memory>
#ifdef DEBUG
#include <iostream>
#endif
#include "Fat32DataAccess.hpp"
#include "LowLevelIO.hpp"

using namespace std;

FileIOError::FileIOError(int ev, const string &what_arg)
  : system_error(ev, system_category(), what_arg) {}
FileIOError::FileIOError(int ev, const char *what_arg)
  : system_error(ev, system_category(), what_arg) {}
FileIOError::FileIOError(error_code ec, const string &what_arg)
  : system_error(ec, "Broken device: " + what_arg) {}
FileIOError::FileIOError(error_code ec, const char *what_arg)
  : system_error(ec, string("Broken device: ") + what_arg) {}
NoMoreData::NoMoreData() : exception() {}
ClusterOccupied::ClusterOccupied() : exception() {}
BrokenFATChain::BrokenFATChain() : exception() {}

const uint8_t FileHandler::FileIsDir = 0x10;
FileHandler::FileHandler() throw()
  : isDir(false),
    isDel(true),
    fstClus(0),
    size(0),
    offset(0),
    dirClus(0),
    dirOffset(0) {}
FileHandler::FileHandler(const string &sName, const string &lName, bool _isDel,
                         bool _isDir, uint32_t _fstClus, uint32_t _size,
                         uint32_t _dirClus, uint32_t _dirOffset, list<uint32_t> &_dirLFNOffsets) throw()
  : shortName(sName),
    longName(lName),
    isDir(_isDir),
    isDel(_isDel),
    fstClus(_fstClus),
    size(_size),
    offset(0),
    dirClus(_dirClus),
    dirOffset(_dirOffset),
    dirLFNOffsets(_dirLFNOffsets)
{
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Constructing FileHandler"
       << ", short name: " << shortName << ", long name: " << longName
       << ", is directory: " << isDir << ", is deleted: " << isDel
       << ", first cluster: " << fstClus << ", size: " << size
       << ", parent directory cluster: " << dirClus
       << ", parent directory offset: " << dirOffset;
  cout << ", parent directory LFN offsets: ";
  for (list<uint32_t>::const_iterator it = dirLFNOffsets.begin(); it != dirLFNOffsets.end(); ++it) {
    cout << *it << ", ";
  }
  cout << endl;
  cout << "\x1b[0m";
#endif //DEBUG
}
FileHandler::FileHandler(uint32_t _fstClus, uint32_t _offset) throw()
  : isDir(true),
    isDel(false),
    fstClus(_fstClus),
    size(0),
    offset(_offset),
    dirClus(0),
    dirOffset(0)
{
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Constructing FileHandler"
       << ", short name: " << shortName << ", long name: " << longName
       << ", is directory: " << isDir << ", is deleted: " << isDel
       << ", first cluster: " << fstClus << ", size: " << size
       << ", parent directory cluster: " << dirClus
       << ", parent directory offset: " << dirOffset
       << ", parent directory LFN offsets: " << endl;
  cout << "\x1b[0m";
#endif //DEBUG
}

bool FileHandler::hasLongName()
{
  return (longName.length() != 0);
}
string FileHandler::getShortName()
{
  return shortName;
}
string FileHandler::getLongName()
{
  return longName;
}
bool FileHandler::isDirectory()
{
  return isDir;
}
bool FileHandler::isDeleted()
{
  return isDel;
}
uint32_t FileHandler::getFstClus()
{
  return fstClus;
}
uint32_t FileHandler::getSize()
{
  return size;
}
uint32_t FileHandler::getOffset()
{
  return offset;
}
void FileHandler::setOffset(uint32_t of)
{
  offset = of;
}
uint32_t FileHandler::getDirClus()
{
  return dirClus;
}
uint32_t FileHandler::getDirOffset()
{
  return dirOffset;
}
const list<uint32_t> & FileHandler::getDirLFNOffsets()
{
  return dirLFNOffsets;
}

string FileHandler::toString()
{
  string ret;
#ifdef DEBUG

  if (isDel) {
    ret.append("[Z] ");
  }

#endif //DEBUG
  ret.append(shortName);

  if (isDir) {
    ret.append("/");
  }

  ret.append(", ");

  if (longName.length() != 0) {
    ret.append(longName);

    if (isDir) {
      ret.append("/");
    }

    ret.append(", ");
  }

  ret.append(to_string(size));
  ret.append(", ");
  ret.append(to_string(fstClus));
  return ret;
}
const uint32_t Fat32DataAccess::FATEntryMask = 0x0FFFFFFF;
const uint32_t Fat32DataAccess::FATEOFClus = 0x0FFFFFF8; //EOF: >=0x0FFFFFF8
const uint32_t Fat32DataAccess::FATFreeClus = 0x00000000;

const uint8_t Fat32DataAccess::DirEntryLFNAttr = 0x0F;
const uint8_t Fat32DataAccess::DirEntryDirAttr = 0x10;

const uint8_t Fat32DataAccess::DirEntryDeleteFlag = 0xE5;
const uint8_t Fat32DataAccess::DirEntryEmptyFlag = 0x00;
const uint8_t Fat32DataAccess::DirEntry83EndFlag = 0x20;
const uint16_t Fat32DataAccess::DirEntryLFNEndFlag = 0x00;

const uint8_t Fat32DataAccess::DirEntryIsEmpty = 0x00;
const uint8_t Fat32DataAccess::DirEntryIsDeleted = 0x01;
const uint8_t Fat32DataAccess::DirEntryIsLFN = 0x02;
const uint8_t Fat32DataAccess::DirEntryIsSFN = 0x04;

Fat32DataAccess::Fat32DataAccess(const string &devName) throw(FileIOError)
  : deviceFd(-1)
{
  //Detecting endianess first
  if ((uint16_t) 1 == le16toh((uint16_t) 1)) {
    isLittleEndian = true;
  }

  deviceFd = open(devName.c_str(), O_RDWR);

  if (-1 == deviceFd) {
    throw FileIOError(errno, devName);
  }

  BootSector bootSector;
  readBootSector(bootSector);
  bytsPerSec = bootSector.BPB_BytsPerSec;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Bytes Per Sector:" << bytsPerSec << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  secPerClus = bootSector.BPB_SecPerClus;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Sector Per Cluster:" << secPerClus << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  bytsPerClus = bootSector.BPB_BytsPerSec * bootSector.BPB_SecPerClus;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Bytes Per Cluster:" << bytsPerClus << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  numFATs = bootSector.BPB_NumFATs;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Number of FATs:" << numFATs << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  rsvdSecCnt = bootSector.BPB_RsvdSecCnt;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Reserved Sectors:" << rsvdSecCnt << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  fatOffset = (uintmax_t) bootSector.BPB_RsvdSecCnt * bootSector.BPB_BytsPerSec;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "First FAT offset:" << fatOffset << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  bytsPerFat = (uintmax_t) bootSector.BPB_FATSz32 * bootSector.BPB_BytsPerSec;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Bytes Per FAT:" << bytsPerFat << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  dataOffset = (uintmax_t)(bootSector.BPB_RsvdSecCnt +
                           bootSector.BPB_NumFATs * bootSector.BPB_FATSz32) *
               bootSector.BPB_BytsPerSec;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Data offset:" << dataOffset << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  totSecCnt = bootSector.BPB_TotSec32 + bootSector.BPB_TotSec16;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Total Sectors:" << totSecCnt << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  totClusCnt = (totSecCnt - bootSector.BPB_RsvdSecCnt -
                bootSector.BPB_NumFATs * bootSector.BPB_FATSz32) / secPerClus;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Total Clusters:" << totClusCnt << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  maxDirEntryPerClus = bytsPerClus / sizeof(DirEntry);
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "DirEntry Per Cluster:" << maxDirEntryPerClus << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  rootClusNo = bootSector.BPB_RootClus;
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "Root Directory Cluster No.:" << rootClusNo << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  list<uint32_t> tmp;
  rootHandler =
    FileHandler("/", "/", false, true, rootClusNo, 0, rootClusNo, 0, tmp);
  readFAT();
}
Fat32DataAccess::~Fat32DataAccess() throw()
{
  if (-1 != deviceFd) {
    close(deviceFd);
  }
}
void Fat32DataAccess::readBootSector(BootSector &bootSector) throw(
  FileIOError)
{
  try {
    LowLevelIO::xpread(deviceFd, &bootSector, sizeof(BootSector), 0);
  } catch (LLIOError &e) {
    throw FileIOError(e.code(), "Reading BootSector");
  } catch (LLIOEOF &e) {
    throw FileIOError(EIO, "EOF when reading BootSector");
  }

  if (!isLittleEndian) {
    bootSector.BPB_BytsPerSec = le16toh(bootSector.BPB_BytsPerSec);
    bootSector.BPB_RsvdSecCnt = le16toh(bootSector.BPB_RsvdSecCnt);
    bootSector.BPB_RootEntCnt = le16toh(bootSector.BPB_RootEntCnt);
    bootSector.BPB_TotSec16 = le16toh(bootSector.BPB_TotSec16);
    bootSector.BPB_FATSz16 = le16toh(bootSector.BPB_FATSz16);
    bootSector.BPB_SecPerTrk = le16toh(bootSector.BPB_SecPerTrk);
    bootSector.BPB_NumHeads = le16toh(bootSector.BPB_NumHeads);
    bootSector.BPB_HiddSec = le32toh(bootSector.BPB_HiddSec);
    bootSector.BPB_TotSec32 = le32toh(bootSector.BPB_TotSec32);
    bootSector.BPB_FATSz32 = le32toh(bootSector.BPB_FATSz32);
    bootSector.BPB_ExtFlags = le16toh(bootSector.BPB_ExtFlags);
    bootSector.BPB_FSVer = le16toh(bootSector.BPB_FSVer);
    bootSector.BPB_RootClus = le32toh(bootSector.BPB_RootClus);
    bootSector.BPB_FSInfo = le16toh(bootSector.BPB_FSInfo);
    bootSector.BPB_BkBootSec = le16toh(bootSector.BPB_BkBootSec);
    bootSector.BS_VolID = le32toh(bootSector.BS_VolID);
  }
}

void Fat32DataAccess::readFAT() throw(FileIOError)
{
  fatMap.clear();
  unique_ptr<uint32_t[]> fat(new uint32_t[bytsPerFat]);

  try {
    LowLevelIO::xpread(deviceFd, fat.get(), bytsPerFat, fatOffset);
  } catch (LLIOError &e) {
    throw FileIOError(e.code(), "Reading FAT table");
  } catch (LLIOEOF &e) {
    throw FileIOError(EIO, "Unexpected EOF when reading FAT table");
  }
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "===========" << endl;
  cout << "FAT entries" << endl;
  cout << "\x1b[0m";
#endif // DEBUG
  for (uint32_t i = 0; i < totClusCnt + 2; i++) {
    if (!isLittleEndian) {
      fat[i] = le32toh(fat[i]);
    }

    fat[i] = fat[i] & FATEntryMask;

    if (FATFreeClus != fat[i]) {
#ifdef DEBUG
  cout << "\x1b[7m";
      cout << i << ": 0x";
      cout << hex;
      cout.width(8);
      cout.fill('0');
      cout << fat[i];
      cout.width(0);
      cout.fill(' ');
      cout << dec << endl;
  cout << "\x1b[0m";
#endif // DEBUG
      fatMap[i] = fat[i];
    }
  }
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << "===========" << endl;
  cout << "\x1b[0m";
#endif // DEBUG
}
/*
 * Recover the file pointed to by fh
 * Rename the first character in SFN entry to name0
 * Assumption: name0 is character/numeric
 */
void Fat32DataAccess::recover(FileHandler &fh,
                              char name0, bool recoverLFN) throw(ClusterOccupied,
                                  BrokenFATChain)
{
  if (!isalnum(name0)) {
    throw logic_error("The first character should be alphanumeric");
  }
  name0 = (char)toupper(name0);
  if (!fh.isDeleted()) {
    throw logic_error("Cannot recover an existing entry");
  }

  if (fh.isDirectory()) {
    throw logic_error("Cannot recover a directory");
  }

  if (fh.getSize() > bytsPerClus) {
    throw BrokenFATChain();
  }

  if (!isFreeClus(fh.getFstClus()) && !isFreeClus(getNextClus(fh.getFstClus()))) {
    throw ClusterOccupied();
  }

  FileHandler dfh(fh.getDirClus(), fh.getDirOffset());
  fs32write(dfh, &name0, 1);
  if (recoverLFN && !fh.getDirLFNOffsets().empty()) {
    uint8_t buf = 1;
    list<uint32_t>::const_iterator lastIt = fh.getDirLFNOffsets().end();
    --lastIt;
    for (list<uint32_t>::const_iterator it = fh.getDirLFNOffsets().begin(); it != fh.getDirLFNOffsets().end(); ++it) {
      FileHandler lfh(fh.getDirClus(), *it);
      if (it==lastIt) {
        buf = buf | 0x40;
      }
      fs32write(lfh, &buf, 1);
      ++buf;
    }
  }
  setNextClus(fh.getFstClus(), FATEOFClus);
}
ssize_t Fat32DataAccess::fs32write(FileHandler &fh, void *buf,
                                   size_t count) throw(FileIOError)
{
  if (0 == count) {
    return 0;
  }

  if (fh.isDeleted()) {
    throw logic_error("Cannot write to a deleted file");
  }

  if (fh.isDirectory()) {
  } else {
    if (fh.getOffset() >= fh.getSize()) {
      return 0;
    }

    if (fh.getOffset() + count > fh.getSize()) {
      count = fh.getSize() - fh.getOffset();
    }
  }

  ssize_t ret = 0;
  off_t offset = fh.getOffset();
  uint32_t clusNo = fh.getFstClus();

  while (offset >= (off_t)bytsPerClus) {
    offset -= bytsPerClus;
    clusNo = getNextClus(clusNo);
  }

  do {
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "Remaining bytes " << count << endl;
  cout << "\x1b[0m";
#endif //DEBUG
    size_t realCount = 0;

    if (offset + count >= bytsPerClus) {
      realCount = bytsPerClus - offset;
    } else {
      realCount = count;
    }

    try {
      LowLevelIO::xpwrite(deviceFd, buf, count, offset + getClusOffset(clusNo));
    } catch (LLIOError &e) {
      throw FileIOError(e.code(), "f32write");
    } catch (LLIOEOF &e) {
      throw FileIOError(EIO, "Unexpected EOF in f32write");
    }

    buf = (unsigned char *)buf + realCount;
    count -= realCount;
    offset += realCount;
    ret += realCount;

    if (offset == (off_t)bytsPerClus) {
      offset = 0;
      clusNo = getNextClus(clusNo);
    }
  } while (0 != count);

  fh.setOffset(fh.getOffset() + ret);
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << ret << "bytes wrote" << endl;
  cout << "\x1b[0m";
#endif //DEBUG
  return ret;
}
ssize_t Fat32DataAccess::fs32read(FileHandler &fh, void *buf,
                                  size_t count) throw(FileIOError,
                                      ClusterOccupied,
                                      BrokenFATChain)
{
  if (fh.isDirectory()) {
    throw logic_error("fs32read: Cannot read directory");
  }

  if (fh.getOffset() >= fh.getSize()) {
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "EOF" << endl;
  cout << "\x1b[0m";
#endif //DEBUG
    return 0;
  }

  if (fh.getSize() == 0) {
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "EOF" << endl;
  cout << "\x1b[0m";
#endif //DEBUG
    return 0;
  }

  if (fh.getOffset() + count > fh.getSize()) {
    count = fh.getSize() - fh.getOffset();
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "fs32read: offset+count>size. Reset count to " << count << endl;
  cout << "\x1b[0m";
#endif //DEBUG
  }

  if (fh.isDeleted()) {
    if (fh.getSize() > bytsPerClus) {
#ifdef DEBUG
  cout << "\x1b[7m";
      cout << "fs32read: Deleted file spanning across multiple clusters"
           << endl;
  cout << "\x1b[0m";
#endif //DEBUG
      throw BrokenFATChain();
    }

    if (!isFreeClus(getNextClus(fh.getFstClus()))) {
#ifdef DEBUG
  cout << "\x1b[7m";
      cout << "fs32read: Deleted file has been overwritten" << endl;
  cout << "\x1b[0m";
#endif //DEBUG
      throw ClusterOccupied();
    }
  }

  ssize_t ret = 0;
  off_t offset = fh.getOffset();
  uint32_t clusNo = fh.getFstClus();

  while (offset >= (off_t)bytsPerClus) {
    offset -= bytsPerClus;
    clusNo = getNextClus(clusNo);
  }

  do {
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "Remaining bytes " << count << endl;
  cout << "\x1b[0m";
#endif //DEBUG
    size_t realCount = 0;

    if (offset + count > (off_t)bytsPerClus) {
      realCount = bytsPerClus - offset;
    } else {
      realCount = count;
    }

    try {
#ifdef DEBUG
  cout << "\x1b[7m";
      cout << "...Read " << realCount << "bytes at device offset "
           << offset + getClusOffset(clusNo) << "B " << endl;
  cout << "\x1b[0m";
#endif //DEBUG
      LowLevelIO::xpread(deviceFd, buf, realCount,
                         offset + getClusOffset(clusNo));
    } catch (LLIOError &e) {
      throw FileIOError(e.code(), "f32read");
    } catch (LLIOEOF &e) {
      throw FileIOError(EIO, "Unexpected EOF in f32read");
    }

    buf = (unsigned char *)buf + realCount;
    count -= realCount;
    offset += realCount;
    ret += realCount;

    if (offset == (off_t)bytsPerClus) {
      offset = 0;
      clusNo = getNextClus(clusNo);
    }
  } while (0 != count);

  fh.setOffset(fh.getOffset() + ret);
#ifdef DEBUG
  cout << "\x1b[7m";
  cout << ret << "bytes read" << endl;
  cout << "\x1b[0m";
#endif //DEBUG
  return ret;
}
uint8_t Fat32DataAccess::readDirEntry(FileHandler &dh,
                                      DirEntry &de) throw(FileIOError,
                                          NoMoreData)
{
  if (!dh.isDirectory() || dh.isDeleted()) {
    throw logic_error("FileHandler is not a directory or is deleted");
  }

  uint32_t offset = dh.getOffset();
  uint32_t clusNo = dh.getFstClus();

  if (offset % sizeof(de) != 0) {
    throw logic_error("Invalid offset for DirEntry");
  }

  while (offset >= bytsPerClus && !isEOFClus(clusNo)) {
    offset -= bytsPerClus;
    clusNo = getNextClus(clusNo);
  }

  if (isEOFClus(clusNo)) {
    throw NoMoreData();
  }

  if (clusNo < 2 || clusNo >= totClusCnt || isFreeClus(getNextClus(clusNo))) {
    throw logic_error("Cluster index outof range");
  }

  uint32_t realOffset = getClusOffset(clusNo) + offset;

  try {
    LowLevelIO::xpread(deviceFd, &de, sizeof(de), realOffset);
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "Read from cluster " << clusNo << " offset " << offset << "B "
         << sizeof(de) << " bytes" << endl;
  cout << "\x1b[0m";
#endif
  } catch (LLIOError &e) {
    throw FileIOError(e.code(), "Reading DirEntry");
  } catch (LLIOEOF &e) {
    throw FileIOError(EIO, "Unexpected EOF when reading DirEntry");
  }

  dh.setOffset(dh.getOffset() + sizeof(de));

  if (DirEntryEmptyFlag == de.raw.status) {
    return DirEntryIsEmpty;
  }

  if (DirEntryLFNAttr == de.raw.attr) {
    if (!isLittleEndian) {
      for (uint32_t i = 0; i < sizeof(de.lfn.name0_4); ++i) {
        de.lfn.name0_4[i] = le16toh(de.lfn.name0_4[i]);
      }

      for (uint32_t i = 0; i < sizeof(de.lfn.name5_10); ++i) {
        de.lfn.name5_10[i] = le16toh(de.lfn.name5_10[i]);
      }

      for (uint32_t i = 0; i < sizeof(de.lfn.name11_12); ++i) {
        de.lfn.name11_12[i] = le16toh(de.lfn.name11_12[i]);
      }
    }

    if (DirEntryDeleteFlag == de.raw.status) {
      return DirEntryIsLFN | DirEntryIsDeleted;
    }

    return DirEntryIsLFN;
  } else {
    if (!isLittleEndian) {
      de.sfn.DIR_CrtTime = le16toh(de.sfn.DIR_CrtTime);
      de.sfn.DIR_CrtDate = le16toh(de.sfn.DIR_CrtDate);
      de.sfn.DIR_LstAccDate = le16toh(de.sfn.DIR_LstAccDate);
      de.sfn.DIR_FstClusHI = le16toh(de.sfn.DIR_FstClusHI);
      de.sfn.DIR_WrtTime = le16toh(de.sfn.DIR_WrtTime);
      de.sfn.DIR_WrtDate = le16toh(de.sfn.DIR_WrtDate);
      de.sfn.DIR_FstClusLO = le16toh(de.sfn.DIR_FstClusLO);
      de.sfn.DIR_FileSize = le32toh(de.sfn.DIR_FileSize);
    }

    if (DirEntryDeleteFlag == de.raw.status) {
      return DirEntryIsSFN | DirEntryIsDeleted;
    }

    return DirEntryIsSFN;
  }
}
FileHandler Fat32DataAccess::getNextFileHandlerFromDir(FileHandler &dh) throw(
  FileIOError, NoMoreData)
{
  if (!dh.isDirectory() && !dh.isDeleted()) {
    throw logic_error("File handler is not a directory or is deleted");
  }
  const int WaitingDeleted = 2;
  const int WaitingUnDel = 1;
  const int WaitingAny = 0;
  int state = WaitingAny;

  DirEntry de;
  list<DirEntry> leList;
  list<uint32_t> leOffsetList;

  //XXX : pissible error here: it works if LFN and 8.3 entries are in order and
  //adjacent.
  while (true) {
    uint8_t dirEntryType;
    dirEntryType = readDirEntry(dh, de);
#ifdef DEBUG
    cout << "\x1b[7m";
    cout << "DirEntry Type: 0x" << hex << (int) dirEntryType << dec << endl;
    cout << "\x1b[0m";
#endif //DEBUG

    if (DirEntryIsEmpty == dirEntryType) {
#ifdef DEBUG
      cout << "\x1b[7m";
      cout << "DirEntry is not used" << endl;
      cout << "Cleaning LFN entry list" << endl;
      cout << "\x1b[0m";
#endif //DEBUG
      leList.clear();
      leOffsetList.clear();
      state = WaitingAny;
      continue;
    } else if (DirEntryIsDeleted & dirEntryType) {
      if (WaitingUnDel == state) {
#ifdef DEBUG
        cout << "\x1b[7m";
        cout << "Cleaning LFN entry list" << endl;
        cout << "\x1b[0m";
#endif //DEBUG
        leList.clear();
        leOffsetList.clear();
      }
      state = WaitingDeleted;
    } else  {
      if (WaitingDeleted == state) {
#ifdef DEBUG
        cout << "\x1b[7m";
        cout << "Cleaning LFN entry list" << endl;
        cout << "\x1b[0m";
#endif //DEBUG
        leList.clear();
        leOffsetList.clear();
      }
      state = WaitingUnDel;
    }

    if (DirEntryIsLFN & dirEntryType) {
#ifdef DEBUG
      cout << "\x1b[7m";
      cout << "DirEntry is LFN" << endl;
      cout << "\x1b[0m";
#endif //DEBUG

      leList.push_front(de);
      leOffsetList.push_front(dh.getOffset() - sizeof(de));
    } else if (DirEntryIsSFN & dirEntryType) {
#ifdef DEBUG
      cout << "\x1b[7m";
      cout << "DirEntry is SFN" << endl;
      cout << "\x1b[0m";
#endif //DEBUG
      break;
    }
  }

  return buildFileHandler(de, leList, dh.getFstClus(),
                          dh.getOffset() - sizeof(de), leOffsetList);
}
FileHandler Fat32DataAccess::buildFileHandler(
    DirEntry &de, list<DirEntry> &leList, uint32_t dirClus, uint32_t dirOffset,
    list<uint32_t> &dirLFNOffsets) throw() {
  string longName;

  for (list<DirEntry>::iterator it = leList.begin(); it != leList.end(); ++it) {
    longName = longName.append(getLongNameSegLFN(*it));
  }

  uint32_t fstClus = (uint32_t) de.sfn.DIR_FstClusHI;
  fstClus = fstClus << 16;
  fstClus += (uint32_t) de.sfn.DIR_FstClusLO;
  return FileHandler(
      getShortNameSFN(de), longName, de.raw.status == DirEntryDeleteFlag,
      de.raw.attr & DirEntryDirAttr, fstClus, de.sfn.DIR_FileSize, dirClus,
      dirOffset, dirLFNOffsets);
}
string Fat32DataAccess::getShortNameSFN(DirEntry &de) throw() {
  string sName, sExt;
  int size = sizeof(de.sfn.DIR_Name) / sizeof(de.sfn.DIR_Name[0]);

  for (int i = 0; i < size; i++) {
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "DIR_Name[" << i << "]=" << hex << de.sfn.DIR_Name[i] << dec
         << endl;
  cout << "\x1b[0m";
#endif

    if (DirEntry83EndFlag == de.sfn.DIR_Name[i]) {
      break;
    } else {
      sName.push_back(de.sfn.DIR_Name[i]);
    }
  }

  size = sizeof(de.sfn.DIR_Ext) / sizeof(de.sfn.DIR_Ext[0]);

  for (int i = 0; i < size; i++) {
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "DIR_Ext[" << i << "]=" << hex << de.sfn.DIR_Ext[i] << dec << endl;
  cout << "\x1b[0m";
#endif

    if (DirEntry83EndFlag == de.sfn.DIR_Ext[i]) {
      break;
    } else {
      sExt.push_back(de.sfn.DIR_Ext[i]);
    }
  }

  if (sExt.length() != 0) {
    sName.append(".").append(sExt);
  }

  return sName;
}
string Fat32DataAccess::getLongNameSegLFN(DirEntry &le) throw()
{
  string lfnSeg;
  int size = sizeof(le.lfn.name0_4) / sizeof(le.lfn.name0_4[0]);

  for (int i = 0; i < size; i++) {
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "name0_4[" << i << "]=" << hex << le.lfn.name0_4[i] << dec << endl;
  cout << "\x1b[0m";
#endif

    if (DirEntryLFNEndFlag == le.lfn.name0_4[i]) {
      return lfnSeg;
    }

    lfnSeg.push_back((char) le.lfn.name0_4[i]);
  }

  size = sizeof(le.lfn.name5_10) / sizeof(le.lfn.name5_10[0]);

  for (int i = 0; i < size; i++) {
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "name5_10[" << i << "]=" << hex << le.lfn.name5_10[i] << dec
         << endl;
  cout << "\x1b[0m";
#endif

    if (DirEntryLFNEndFlag == le.lfn.name5_10[i]) {
      return lfnSeg;
    }

    lfnSeg.push_back((char) le.lfn.name5_10[i]);
  }

  size = sizeof(le.lfn.name11_12) / sizeof(le.lfn.name11_12[0]);

  for (int i = 0; i < size; i++) {
#ifdef DEBUG
  cout << "\x1b[7m";
    cout << "name11_12[" << i << "]=" << hex << le.lfn.name11_12[i] << dec
         << endl;
  cout << "\x1b[0m";
#endif

    if (DirEntryLFNEndFlag == le.lfn.name11_12[i]) {
      return lfnSeg;
    }

    lfnSeg.push_back((char) le.lfn.name11_12[i]);
  }

  return lfnSeg;
}

FileHandler Fat32DataAccess::getRootHandler() throw()
{
  return rootHandler;
}
bool Fat32DataAccess::isFreeClus(uint32_t clusNo) throw()
{
  if (FATFreeClus == clusNo) {
    return true;
  } else {
    return false;
  }
}
bool Fat32DataAccess::isEOFClus(uint32_t clusNo) throw()
{
  if (FATEOFClus <= clusNo) {
    return true;
  } else {
    return false;
  }
}
uint32_t Fat32DataAccess::getNextClus(uint32_t clusNo) throw()
{
  if (clusNo < 2 || clusNo >= totClusCnt) {
    throw logic_error("getNextClus: Cluster index outof range");
  }

  if (0 == fatMap.count(clusNo)) {
    return FATFreeClus;
  } else {
    return fatMap[clusNo];
  }
}
uint32_t Fat32DataAccess::getClusOffset(uint32_t clusNo) throw()
{
  if (clusNo < 2 || clusNo >= totClusCnt) {
    throw logic_error("getClusOffset: Cluster index outof range");
  } else {
    return dataOffset + bytsPerClus * (clusNo - 2);
  }
}
void Fat32DataAccess::setNextClus(uint32_t curClus,
                                  uint32_t nextClus) throw(FileIOError)
{
  if (curClus == 0) {
    return;
  }
  if (!isEOFClus(nextClus) && (curClus < 2 || curClus >= totClusCnt)) {
    throw logic_error("setNextClus: Cluster index outof range");
  }

  if (!isEOFClus(nextClus) && nextClus >= totClusCnt) {
    throw logic_error("setNextClus: Cluster index outof range");
  }

  fatMap[curClus] = nextClus;
  vector<off_t> offsets;
  for (uint32_t i =0; i < numFATs; ++i ) {
    offsets.push_back(fatOffset + i * bytsPerFat + curClus * sizeof(uint32_t));
  }
  uint32_t nextClusLE = htole32(nextClus);

  try {
    while (!offsets.empty()) {
      off_t offset = offsets.back();
      offsets.pop_back();
      LowLevelIO::xpwrite(deviceFd, &nextClusLE, sizeof(uint32_t), offset);
    }
  } catch (LLIOError &e) {
    throw FileIOError(e.code(), "Writing FAT table");
  }
}

uint32_t Fat32DataAccess::getBytsPerSec() throw()
{
  return (uint32_t) bytsPerSec;
}
uint32_t Fat32DataAccess::getSecPerClus() throw()
{
  return (uint32_t) secPerClus;
}
uint32_t Fat32DataAccess::getRsvdSecCnt() throw()
{
  return (uint32_t) rsvdSecCnt;
}
uint32_t Fat32DataAccess::getNumFATs() throw()
{
  return (uint32_t) numFATs;
}
uint32_t Fat32DataAccess::getTotClusCnt() throw()
{
  return totClusCnt;
}
uint32_t Fat32DataAccess::getFreeClusCnt() throw()
{
  return totClusCnt - fatMap.size() + 2;
}
uint32_t Fat32DataAccess::getAllocClusCnt() throw()
{
  return fatMap.size() - 2;
}

