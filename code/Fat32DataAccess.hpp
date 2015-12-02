#ifndef FAT32DATAACCESS_HPP
#define FAT32DATAACCESS_HPP

#include <stdint.h>
#include <system_error>
#include <map>
#include <list>
#include <memory>
using namespace std;
class FileIOError : public system_error
{
public:
  explicit FileIOError(int ev, const string &what_arg);
  explicit FileIOError(int ev, const char *what_arg);
  explicit FileIOError(error_code ec, const string &what_arg);
  explicit FileIOError(error_code ec, const char *what_arg);
};
class NoMoreData : public exception
{
public:
  explicit NoMoreData();
};
class ClusterOccupied : public exception
{
public:
  explicit ClusterOccupied();
};
class BrokenFATChain : public exception
{
public:
  explicit BrokenFATChain();
};
class FileHandler
{
private:
  string shortName;
  string longName;
  bool isDir;
  bool isDel;
  uint32_t fstClus;
  uint32_t size;   //no use for directory
  uint32_t offset; //relative to start of file
  uint32_t dirClus;
  uint32_t dirOffset;
  list<uint32_t> dirLFNOffsets;

public:
  static const uint8_t FileIsDir;
  FileHandler() throw();
  FileHandler(const string &sName, const string &lName, bool isDel, bool isDir,
              uint32_t _fstClus, uint32_t _size, uint32_t _dirClus,
              uint32_t _dirOffset, list<uint32_t> &_dirLFNOffsets) throw();
  FileHandler(uint32_t _fstClus, uint32_t _offset) throw();
  bool hasLongName();
  string getShortName();
  string getLongName();
  bool isDirectory();
  bool isDeleted();
  uint32_t getFstClus();
  uint32_t getSize();
  void setOffset(uint32_t of);
  uint32_t getOffset();
  uint32_t getDirClus();
  uint32_t getDirOffset();
  const list<uint32_t> &getDirLFNOffsets();
  string toString();
};

class Fat32DataAccess
{

private:
  struct BootSector {
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec; // Bytes per sector
    uint8_t BPB_SecPerClus;  // Sectors per cluster
    uint16_t BPB_RsvdSecCnt; // Size in sectors of the reserved area
    uint8_t BPB_NumFATs;     // Number of FATs
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16; // 16-bit value of number of sectors in file system
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec; /* 32-bit value of number of sectors in file system.
            Either this value or the 16-bit value above must be
            0 */
    uint32_t BPB_TotSec32; // 32-bit size in sectors of one FAT
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus; // Cluster where the root directory can be found
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];
  } __attribute__((__packed__));
  union DirEntry {
    struct SFNEntry {
      uint8_t DIR_Name[8]; //8 Name
      uint8_t DIR_Ext[3];  //3 Extendion
      uint8_t DIR_Attr;
      uint8_t DIR_NTRes;
      uint8_t DIR_CrtTimeTenth;
      uint16_t DIR_CrtTime;
      uint16_t DIR_CrtDate;
      uint16_t DIR_LstAccDate;
      uint16_t DIR_FstClusHI; //First cluster HI 2 bytes
      uint16_t DIR_WrtTime;
      uint16_t DIR_WrtDate;
      uint16_t DIR_FstClusLO; //First cluster LOW 2 bytes
      uint32_t DIR_FileSize;  //File Size

    } __attribute__((__packed__)) sfn;
    struct LFNEntry {
      uint8_t id;             /* sequence number for slot */
      uint16_t name0_4[5];    /* first 5 characters in name */
      uint8_t attr;           /* attribute byte */
      uint8_t reserved;       /* always 0 */
      uint8_t alias_checksum; /* checksum for 8.3 alias */
      uint16_t name5_10[6];   /* 6 more characters in name */
      uint16_t start;         /* starting cluster number, 0 in long slots */
      uint16_t name11_12[2];  /* last 2 characters in name */
    } __attribute__((__packed__)) lfn;
    struct RawEntry {
      uint8_t status; /*Never touched: 0x00 ;
               *Deleted : 0xE5;
               *Directory : 0x2e
               *LFN sequence : 0x01...
               */
      uint8_t dontcare1[10];
      uint8_t attr;
      uint8_t dontcare2[20];
    } __attribute__((__packed__)) raw;
  };
  void readBootSector(BootSector &bootSector) throw(FileIOError);
  void readFAT() throw(FileIOError);
  uint8_t readDirEntry(FileHandler &dh, DirEntry &de) throw(FileIOError,
      NoMoreData);
  uint32_t getNextClus(uint32_t clusNo) throw();
  uint32_t getClusOffset(uint32_t clusNo) throw();
  bool isFreeClus(uint32_t clusNo) throw();
  bool isEOFClus(uint32_t clusNo) throw();

  void setNextClus(uint32_t curClus, uint32_t nextClus) throw(FileIOError);

  FileHandler buildFileHandler(DirEntry &de, list<DirEntry> &leList,
                               uint32_t dirClus, uint32_t dirOffset,
                               list<uint32_t> &dirLFNOffsets) throw();
  string getShortNameSFN(DirEntry &de) throw();
  string getLongNameSegLFN(DirEntry &le) throw();
  ssize_t fs32write(FileHandler &fh, void *buf,
                    size_t count) throw(FileIOError);

  int deviceFd;
  bool isLittleEndian;
  uintmax_t fatOffset;
  uintmax_t bytsPerFat;
  uintmax_t dataOffset;
  uint32_t totSecCnt;
  uint32_t totClusCnt;
  uint32_t maxDirEntryPerClus;
  map<uint32_t, uint32_t> fatMap;
  FileHandler rootHandler;
  uint32_t rootClusNo;

  uint32_t bytsPerSec;
  uint32_t bytsPerClus;
  uint32_t secPerClus;
  uint32_t numFATs;
  uint32_t rsvdSecCnt;

public:
  static const uint32_t FATEOFClus;
  static const uint32_t FATEntryMask;
  static const uint32_t FATFreeClus;

  static const uint8_t DirEntryLFNAttr;
  static const uint8_t DirEntryDirAttr;

  static const uint8_t DirEntryDeleteFlag;
  static const uint8_t DirEntryEmptyFlag;
  static const uint8_t DirEntry83EndFlag;
  static const uint16_t DirEntryLFNEndFlag;

  static const uint8_t DirEntryIsEmpty;
  static const uint8_t DirEntryIsDeleted;
  static const uint8_t DirEntryIsLFN;
  static const uint8_t DirEntryIsSFN;

  Fat32DataAccess(const string &devName)
  throw(FileIOError);
  ~Fat32DataAccess() throw();

  ssize_t fs32read(FileHandler &fh, void *buf,
                   size_t count) throw(FileIOError, ClusterOccupied,
                                       BrokenFATChain);

  //Milestone 4-6:
  void recover(FileHandler &fh, char name0, bool recoverLFN) throw(ClusterOccupied,
      BrokenFATChain);

  //Milestone 3:
  FileHandler getRootHandler() throw();
  FileHandler getNextFileHandlerFromDir(FileHandler &dh) throw(FileIOError,
      NoMoreData);

  //Milestone 2:
  uint32_t getBytsPerSec() throw();
  uint32_t getSecPerClus() throw();
  uint32_t getRsvdSecCnt() throw();
  uint32_t getNumFATs() throw();
  uint32_t getTotClusCnt() throw();
  uint32_t getFreeClusCnt() throw();
  uint32_t getAllocClusCnt() throw();
};
#endif // FAT32DATAACCESS_HPP
