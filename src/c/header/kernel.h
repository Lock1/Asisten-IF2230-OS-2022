// Kernel header

#include "std_datatype.h"
#include "std_lib.h"

extern void putInMemory(int segment, int address, char character);
extern int interrupt(int int_number, int AX, int BX, int CX, int DX);

void makeInterrupt21();
void handleInterrupt21(int AX, int BX, int CX, int DX);
void printString(char *string);
void readString(char *string);
void clearScreen();

void writeSector(byte *buffer, int sector_number);
void readSector(byte *buffer, int sector_number);

void write(byte *buffer, char *node_name, enum fs_retcode *return_code, byte parent_index);
void read(byte *buffer, char *node_name, enum fs_retcode *return_code, byte parent_index);
