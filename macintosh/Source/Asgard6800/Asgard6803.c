//###################################################################################################
//
//
//		Asgard6803.c
//		Interface file to make a standalone Asgard6803 core.
//
//		See Asgard6800Core.c for information about the Asgard6800 system.
//
//
//###################################################################################################

#include "Asgard6803.h"

// map all function names and global variables
#define gAsgard6800ICount			gAsgard6803ICount
#define Asgard6800Reset				Asgard6803Reset
#define Asgard6800SetReg			Asgard6803SetReg
#define Asgard6800SetRegs			Asgard6803SetRegs
#define Asgard6800GetReg			Asgard6803GetReg
#define Asgard6800GetRegs			Asgard6803GetRegs
#define Asgard6800GetPC				Asgard6803GetPC
#define Asgard6800SetNMILine		Asgard6803SetNMILine
#define Asgard6800SetIRQLine		Asgard6803SetIRQLine
#define Asgard6800SetIRQCallback	Asgard6803SetIRQCallback
#define Asgard6800Execute			Asgard6803Execute

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
