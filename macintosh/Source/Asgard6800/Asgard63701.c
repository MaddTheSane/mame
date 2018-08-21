//###################################################################################################
//
//
//		Asgard63701.c
//		Interface file to make a standalone Asgard63701 core.
//
//		See Asgard6800Core.c for information about the Asgard6800 system.
//
//
//###################################################################################################

#include "Asgard63701.h"

// map all function names and global variables
#define gAsgard6800ICount			gAsgard63701ICount
#define Asgard6800Reset				Asgard63701Reset
#define Asgard6800SetReg			Asgard63701SetReg
#define Asgard6800SetRegs			Asgard63701SetRegs
#define Asgard6800GetReg			Asgard63701GetReg
#define Asgard6800GetRegs			Asgard63701GetRegs
#define Asgard6800GetPC				Asgard63701GetPC
#define Asgard6800SetNMILine		Asgard63701SetNMILine
#define Asgard6800SetIRQLine		Asgard63701SetIRQLine
#define Asgard6800SetIRQCallback	Asgard63701SetIRQCallback
#define Asgard6800Execute			Asgard63701Execute

#include "Asgard6800Core.c"

// now take it all back
#undef Asgard6800Execute
#undef Asgard6800SetIRQCallback
#undef Asgard6800SetIRQLine
#undef Asgard6800SetNMILine
#undef Asgard6800GetPC
#undef Asgard6800GetRegs
#undef Asgard6800GetReg
#undef Asgard6800SetRegs
#undef Asgard6800SetReg
#undef Asgard6800Reset
#undef gAsgard6800ICount
