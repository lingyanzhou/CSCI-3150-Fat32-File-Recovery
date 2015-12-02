CXX=g++
LINK.o=g++
CXXFLAGS=-Wall -std=c++0x
LOADLIBES=-lssl -lcrypto
OBJECTS=recovery.o\
	LowLevelIO.o\
	Fat32RecoveryApp.o\
	Fat32Action.o\
	Fat32DataAccess.o\
	PrintBootSectorInfo.o\
	ListAllDirectoryEntry.o\
	FileRecovery83.o\
	FileRecovery83WithMD5.o\
	FileRecoveryLong.o


.PHONY: release
release: recovery

.PHONY: debug
debug: CXXFLAGS+=-DDEBUG
debug: recovery

recovery: $(OBJECTS)
recovery.o: recovery.cpp Fat32RecoveryApp.hpp Fat32DataAccess.hpp
Fat32RecoveryApp.o: Fat32RecoveryApp.cpp\
	Fat32RecoveryApp.hpp\
	Fat32Action.hpp\
	PrintBootSectorInfo.hpp\
	ListAllDirectoryEntry.hpp\
	FileRecovery83.hpp\
	FileRecovery83WithMD5.hpp\
	FileRecoveryLong.hpp
Fat32DataAccess.o: Fat32DataAccess.cpp Fat32DataAccess.hpp LowLevelIO.hpp
Fat32Action.o: Fat32Action.cpp Fat32Action.hpp Fat32DataAccess.hpp
PrintBootSectorInfo.o: PrintBootSectorInfo.cpp PrintBootSectorInfo.hpp Fat32Action.hpp  Fat32DataAccess.hpp
ListAllDirectoryEntry.o: ListAllDirectoryEntry.cpp ListAllDirectoryEntry.hpp Fat32Action.cpp  Fat32DataAccess.hpp
FileRecovery83.o: FileRecovery83.cpp FileRecovery83.hpp Fat32Action.cpp  Fat32DataAccess.hpp
FileRecovery83WithMD5.o: FileRecovery83WithMD5.cpp FileRecovery83WithMD5.hpp Fat32Action.cpp  Fat32DataAccess.hpp
FileRecoveryLong.o: FileRecoveryLong.cpp FileRecoveryLong.hpp Fat32Action.cpp  Fat32DataAccess.hpp
LowLevelIO.o: LowLevelIO.cpp LowLevelIO.hpp

.PHONY: clean
clean:
	rm $(OBJECTS)
