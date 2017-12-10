/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__ 
#define __USERPROG_KSYSCALL_H__ 

#include "kernel.h"

#include "synchconsole.h"


void SysHalt()
{
  kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename);
}

void SysPrintInt(int number)
{
	kernel->interrupt->PrintInt(number);
}

OpenFileId SysOpen(char *filename)
{
	kernel->interrupt->OpenFile(filename);
}

int SysRead(char *buffer, int size, OpenFileId id)
{
	kernel->interrupt->Read(buffer,size,id);
}

int SysWrite(char *buffer, int size, OpenFileId id)
{
	kernel->interrupt->Write(buffer,size,id);
}

int SysClose(OpenFileId id)
{
	kernel->interrupt->Close(id);
}
#endif /* ! __USERPROG_KSYSCALL_H__ */
