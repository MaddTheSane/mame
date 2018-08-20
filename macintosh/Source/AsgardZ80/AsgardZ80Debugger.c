//###################################################################################################
//
//
//		AsgardZ80Debugger.c
//		Internal debugging system for tracking down incompatibilities with another Z80 core.
//
//		See AsgardZ80Core.c for information about the AsgardZ80 system.
//
//
//###################################################################################################

#include "AsgardZ80Core.h"
#include "AsgardZ80Internals.h"

#if AZ80_COREDEBUG

#include <stdio.h>
#include <string.h>


#undef printf

//###################################################################################################
//	TYPE DEFINITIONS
//###################################################################################################

#pragma options align=packed

typedef struct
{
	unsigned long		fPCAF;
	unsigned long 		fSPDE;
	unsigned long		fHLBC;
	unsigned long		fIXIY;
	unsigned long		fAFDE2;
	unsigned long		fHLBC2;
	unsigned long		fFlags;
	int			 		fICount;
} AsgardZ80IOState;

#pragma options align=reset


//###################################################################################################
//	GLOBAL VARIABLES
//###################################################################################################

// internal counters
static int 		sInstructionCount = 0;
static int 		sInstructionCountDown = 0;

// memory tracking globals
static int 		sPrintMemoryAccesses = 0;
static FILE *	sMemoryCompareLog;
static FILE *	sMemorySaveLog;

// instruction tracking globals
static FILE *	sStateCompareLog;
static FILE *	sStateSaveLog;
static int 		sStateIOStride;
static int		sStateIOCountDown;

// CPU dumping globals
static int		sStateDumpActive;
static FILE *	sStateDumpLog[4];
static int		sStateDumpCount[4];
static int		sStateDumpIndex[4];
static char		sStateDumpLogName[64];


//###################################################################################################
//	IMPLEMENTATION
//###################################################################################################

//###################################################################################################
//
//	rand override -- when we're debugging, we need to have consistent results
//
//###################################################################################################

int rand(void)
{
	static int sValue;
	return (sValue++ & 32767);
}


//###################################################################################################
//
//	AsgardZ80MiniTrace -- update the debugger
//
//	Call this function once per opcode and pass the packed registers in for debugging.
//
//###################################################################################################

void AsgardZ80MiniTrace(unsigned long inPCAF, unsigned long inSPDE, unsigned long inHLBC, unsigned long inIXIY, unsigned long inAFDE2, unsigned long inHLBC2, unsigned long inFlags, int inICount)
{
	AsgardZ80IOState	tempState;
	char 				tempString[80];
	int					i;

	//  only some flags are important	
	inFlags &= 0xff80ffe3;

	//
	//	Simple state dumping to several files
	//
	if (sStateDumpActive)
	{
		int cpu = cpu_getactivecpu();
		
		// every 25000 lines, create a new file
		if (sStateDumpCount[cpu] > 25000)
		{
			if (sStateDumpLog[cpu])
				fclose(sStateDumpLog[cpu]);

			// create a name with the index at the end
			sprintf(tempString, "%s-%d.%03d", sStateDumpLogName, cpu, sStateDumpIndex[cpu]++);
			sStateDumpLog[cpu] = fopen(tempString, "wt");
			
			// reset the count
			sStateDumpCount[cpu] = 0;
		}
		
		// diassemble the current instruction
		DasmZ80(tempString, inPCAF >> 16);
		
		// print it, along with the current state, to the file
		fprintf(sStateDumpLog[cpu], "%6d %08X %08X %08X %08X %08X %08X %08X %s\n", 
				inICount, inSPDE, inHLBC, inIXIY, inAFDE2, inHLBC2, inFlags, inPCAF, tempString);
		sStateDumpCount[cpu]++;
	}

	//
	//	If we're doing full logging, see if it's time to go
	//
	if (sStateIOCountDown != 0)
	{
		// we keep decrementing the down counter until we hit zero; then we process
		if (!--sStateIOCountDown)
		{
			sStateIOCountDown = sStateIOStride;
			
			//
			//	Log output -- dump the regs to an array and write them out
			//
			if (sStateSaveLog)
			{
				tempState.fPCAF = inPCAF;
				tempState.fSPDE = inSPDE;
				tempState.fHLBC = inHLBC;
				tempState.fIXIY = inIXIY;
				tempState.fAFDE2 = inAFDE2;
				tempState.fHLBC2 = inHLBC2;
				tempState.fFlags = inFlags;
				tempState.fICount = inICount;
				fwrite(&tempState, 1, sizeof(tempState), sStateSaveLog);
			}

			//
			//	Log input -- read the expected regs and compare against the current ones
			//
			if (sStateCompareLog)
			{
				if (fread(&tempState, 1, sizeof(tempState), sStateCompareLog) == sizeof(tempState))
				{
					if (tempState.fPCAF    != inPCAF ||
						tempState.fSPDE    != inSPDE ||
						tempState.fHLBC    != inHLBC || 
						tempState.fIXIY    != inIXIY || 
						tempState.fAFDE2   != inAFDE2 || 
						tempState.fHLBC2   != inHLBC2 || 
						(tempState.fFlags & 0xffff)   != (inFlags & 0xffff) ||
						tempState.fICount  != inICount)
					{
						// uh-oh ... a mismatch ... print the state
						sInstructionCountDown = 0;
						printf("Mismatch occurred after %d instructions!  Expected:\n", sInstructionCount);

						// also print a context of 4 instructions
						fseek(sStateCompareLog, -4 * sizeof(tempState), SEEK_CUR);
						for (i = 0; i < 4; i++)
						{
							fread(&tempState, 1, sizeof(tempState), sStateCompareLog);
							DasmZ80(tempString, tempState.fPCAF >> 16);
							printf("%6d SPDE=%08X HLBC=%08X IXIY=%08X AFDE2=%08X HLBC2=%08X FLAGS=%08X PCAF=%08X %s\n", 
									tempState.fICount, tempState.fSPDE, tempState.fHLBC, tempState.fIXIY, 
									tempState.fAFDE2, tempState.fHLBC2, tempState.fFlags,
									tempState.fPCAF, tempString);
						}

						// print a header over the line we're about to print
						printf("Got:\n");
					}
				}
			}
		}
	}

	//
	//	See if we're supposed to keep running
	//
	++sInstructionCount;
	if (sInstructionCountDown)
	{
		--sInstructionCountDown;
		return;
	}

	//
	//	Display the current registers plus a disassembly of the current line
	//
	DasmZ80(tempString, inPCAF >> 16);
	printf("%6d SPDE=%08X HLBC=%08X IXIY=%08X AFDE2=%08X HLBC2=%08X FLAGS=%08X PCAF=%08X %s\n", 
			inICount, inSPDE, inHLBC, inIXIY, inAFDE2, inHLBC2, inFlags, inPCAF, tempString);

	//
	//	Prompt for a command
	//
	tempString[0] = 0;
	while (1)
	{
		char *	name;
		char	c;
		int 	val;
		int		i;
		unsigned long	ticks;

		// cheesy, but it works		
		gets(tempString);
		
		switch (tempString[0])
		{
			//
			//	Enter single-steps
			//
			case 0:
				return;

			//
			//	Help!
			//
			case '?':
				printf("Commands:\n");
				printf("  <nnn>       execute for <nnn> instructions\n");
				printf("  d           drop to debugger\n");
				printf("  l <file>    toggle logging\n");
				printf("  a           display memory accesses\n");
				printf("  m[n] <file> make a tracking file for every n instructions (default=1)\n");
				printf("  c[n] <file> check against a tracking file every n instructions\n");
				printf("  mm <file>   make a memory tracking file\n");
				printf("  cm <file>   check against a memory tracking file\n");
				printf("  c[n] <file> check against a tracking file every n instructions\n");
				printf("  v xxxx      view memory at address xxxx\n");
				printf("  q           quit\n");
				break;
			
			//
			//	Display memory accesses
			//
			case 'a':
				sPrintMemoryAccesses = !sPrintMemoryAccesses;
				printf("Memory access display = %s\n", sPrintMemoryAccesses ? "ON" : "OFF");
				break;
			
			//
			//	Drop to the Mac debugger
			//
			case 'd':
				printf("Debugger\n");
				Debugger();
				return;
			
			//
			//	Turn on real logging
			//
			case 'l':
				name = strchr(tempString, ' ');
				
				// if no name specified, turn off logging
				if (!name)
				{
					if (sStateDumpActive)
					{
						printf("Logging file closed\n");
						
						// make sure each file is properly closed
						for (i = 0; i < 4; i++)
							if (sStateDumpLog[i])
							{
								fclose(sStateDumpLog[i]);
								sStateDumpLog[i] = NULL;
							}
						sStateDumpActive = 0;
					}
				}
				
				// if a name was specified, set it and begin logging
				else
				{
					name++;
					if (!sStateDumpActive)
					{
						printf("Creating new logging files '%s-cpu.count'\n", name);
						sStateDumpActive = 1;
						strcpy(sStateDumpLogName, name);
						
						// make sure the states are all reset to appropriate values
						memset(sStateDumpCount, 1, sizeof(sStateDumpCount));
						memset(sStateDumpIndex, 0, sizeof(sStateDumpIndex));
						memset(sStateDumpLog, 0, sizeof(sStateDumpLog));
					}
				}
				break;

			//
			//	Make a tracking file
			//
			case 'm':

				//
				//	Either memory tracking....
				//
				if (tempString[1] == 'm')
				{
					name = strchr(tempString, ' ');
					
					// no name means close the file and turn it off
					if (!name)
					{
						if (sMemorySaveLog)
						{
							printf("Memory tracking file closed\n");
							fclose(sMemorySaveLog);
							sMemorySaveLog = NULL;
						}
					}

					// a specified name means open the file and turn it on
					else
					{
						name++;
						if (!sMemorySaveLog)
						{
							printf("Creating new memory tracking file '%s'\n", name);
							sMemorySaveLog = fopen(name, "wb");
						}
					}
					break;
				}
				
				//
				//	....or full register tracking
				//
				
				// extract the stride from the characters immediately following
				sStateIOStride = 1;
				if (tempString[1] >= '0' && tempString[1] <= '9')
					sStateIOStride = atoi(tempString+1);
				if (sStateIOStride < 1) 
					sStateIOStride = 1;
				sStateIOCountDown = sStateIOStride;
					
				name = strchr(tempString, ' ');

				// no name means close the file and turn it off
				if (!name)
				{
					if (sStateSaveLog)
					{
						printf("Tracking file closed\n");
						fclose(sStateSaveLog);
						sStateSaveLog = NULL;
						sStateIOCountDown = 0;
					}
				}

				// a specified name means open the file and turn it on
				else
				{
					name++;
					if (!sStateSaveLog)
					{
						printf("Creating new tracking file '%s'\n", name);
						printf("Checking every %d instructions\n", sStateIOStride);
						sStateSaveLog = fopen(name, "wb");
					}
				}
				break;

			//
			//	Compare against a tracking file
			//
			case 'c':

				//
				//	Either memory tracking....
				//
				if (tempString[1] == 'm')
				{
					name = strchr(tempString, ' ');

					// no name means close the file and turn it off
					if (!name)
					{
						if (sMemoryCompareLog)
						{
							printf("Memory tracking file closed\n");
							fclose(sMemoryCompareLog);
							sMemoryCompareLog = NULL;
						}
					}

					// a specified name means open the file and turn it on
					else
					{
						name++;
						if (!sMemoryCompareLog)
						{
							printf("Comparing against memory tracking file '%s'\n", name);
							sMemoryCompareLog = fopen(name, "rb");
							if (!sMemoryCompareLog)
								printf("Error opening file\n");
						}
					}
					break;
				}
				
				//
				//	....or full register tracking
				//

				// extract the stride from the characters immediately following
				sStateIOStride = 1;
				if (tempString[1] >= '0' && tempString[1] <= '9')
					sStateIOStride = atoi(tempString+1);
				if (sStateIOStride < 1) sStateIOStride = 1;
				sStateIOCountDown = sStateIOStride;

				name = strchr(tempString, ' ');

				// no name means close the file and turn it off
				if (!name)
				{
					if (sStateCompareLog)
					{
						printf("Tracking file closed\n");
						fclose(sStateCompareLog);
						sStateCompareLog = NULL;
						sStateIOCountDown = 0;
					}
				}

				// a specified name means open the file and turn it on
				else
				{
					name++;
					if (!sStateCompareLog)
					{
						printf("Comparing against tracking file '%s'\n", name);
						printf("Checking every %d instructions\n", sStateIOStride);
						sStateCompareLog = fopen(name, "rb");
						if (!sStateCompareLog)
							printf("Error opening file\n");
					}
				}
				break;

			//
			//	Quit -- close all the files and exit
			//
			case 'q':
				printf("Quitting\n");
				
				// make sure all the files are close so that we flush properly
				for (i = 0; i < 4; i++)
					if (sStateDumpLog[i])
						fclose(sStateDumpLog[i]);
				if (sStateCompareLog)
					fclose(sStateCompareLog);
				if (sStateSaveLog)
					fclose(sStateSaveLog);
				if (sMemoryCompareLog)
					fclose(sMemoryCompareLog);
				if (sMemorySaveLog)
					fclose(sMemorySaveLog);
					
				// not very nice...
				ExitToShell();
				break;
			
			//
			//	View memory at a given address
			//
			case 'v':
				sscanf(tempString + 1, " %x", &val);
				printf("%04X:", val);
				for (i = 0; i < 16; i++)
					printf(" %02X", READMEM((val + i) & 0xffff));
				printf("\n");
				break;

			//
			//	Run for the specified number of instructions
			//
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				sInstructionCountDown = atoi(tempString);
				
				c = tempString[strlen(tempString)-1];
				if (c == 'm') sInstructionCountDown *= 1000000;
				else if (c == 'k') sInstructionCountDown *= 1000;
				Delay(60, &ticks);
				printf("Running for %d instructions....\n", sInstructionCountDown);
				return;
		}
	}
}


//###################################################################################################
//
//	AsgardZ80DebugRead -- internal debugging memory read that handles tracking and logging
//
//###################################################################################################

int AsgardZ80DebugRead(int inAddress)
{
	int val = AZ80_READMEM(inAddress);
	
	if (sPrintMemoryAccesses) 
		printf("  read %04X = %02X\n", inAddress, val);

	if (sMemoryCompareLog)
	{
		int vals[2];
		fread(vals, 1, sizeof(vals), sMemoryCompareLog);
		if (vals[0] != inAddress || vals[1] != val)
		{
			sInstructionCountDown = 0;
			printf("Mismatch occurred after %d instructions!  Expected:\n", sInstructionCount);
			printf("   Read %04X = %02X\n", vals[0], vals[1]);
			printf("Got:\n");
			printf("   Read %04X = %02X\n", inAddress, val);
		}
	}

	if (sMemorySaveLog)
	{
		int vals[2];
		vals[0] = inAddress;
		vals[1] = val;
		fwrite(vals, 1, sizeof(vals), sMemorySaveLog);
	}

	return val;
}


//###################################################################################################
//
//	AsgardZ80DebugRead -- internal debugging memory write that handles tracking and logging
//
//###################################################################################################

void AsgardZ80DebugWrite(int inAddress, int inValue)
{
	if (sPrintMemoryAccesses) 
		printf("  write %04X = %02X\n", inAddress, inValue);

	if (sMemoryCompareLog)
	{
		int vals[2];
		fread(vals, 1, sizeof(vals), sMemoryCompareLog);
		if (vals[0] != inAddress || vals[1] != inValue)
		{
			sInstructionCountDown = 0;
			printf("Mismatch occurred after %d instructions!  Expected:\n", sInstructionCount);
			printf("   Write %04X = %02X\n", vals[0], vals[1]);
			printf("Got:\n");
			printf("   Write %04X = %02X\n", inAddress, inValue);
		}
	}

	if (sMemorySaveLog)
	{
		int vals[2];
		vals[0] = inAddress;
		vals[1] = inValue & 0xff;
		fwrite(vals, 1, sizeof(vals), sMemorySaveLog);
	}
	AZ80_WRITEMEM(inAddress, inValue);
}

#endif
