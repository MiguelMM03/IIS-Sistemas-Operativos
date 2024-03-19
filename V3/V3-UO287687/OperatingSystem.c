#include "Simulator.h"
#include "OperatingSystem.h"
#include "OperatingSystemBase.h"
#include "MMU.h"
#include "Processor.h"
#include "Buses.h"
#include "Heap.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

// Functions prototypes
void OperatingSystem_PCBInitialization(int, int, int, int, int);
void OperatingSystem_MoveToTheREADYState(int);
void OperatingSystem_Dispatch(int);
void OperatingSystem_RestoreContext(int);
void OperatingSystem_SaveContext(int);
void OperatingSystem_TerminateExecutingProcess();
int OperatingSystem_LongTermScheduler();
void OperatingSystem_PreemptRunningProcess();
int OperatingSystem_CreateProcess(int);
int OperatingSystem_ObtainMainMemory(int, int);
int OperatingSystem_ShortTermScheduler();
int OperatingSystem_ExtractFromReadyToRun();
void OperatingSystem_HandleException();
void OperatingSystem_HandleSystemCall();
void OperatingSystem_HandleClockInterrupt();
void OperatingSystem_MoveToBlockedState();
void OperatingSystem_ChangeProcess(int);
void OperatingSystem_ChangeProcessToMostPriorityProcess();
int OperatingSystem_GetAndNotExtractMostPriorityReadyToRunProcess();
int OperatingSystem_GetExecutingProcessID();

char * statesNames [5]={"NEW","READY","EXECUTING","BLOCKED","EXIT"};

//Número de interrupciones de reloj
int numberOfClockInterrupts=0;

// In OperatingSystem.c Exercise 7-b of V2
// Heap with blocked processes sort by when to wakeup
heapItem *sleepingProcessesQueue;
int numberOfSleepingProcesses=0; 

// The process table
// PCB processTable[PROCESSTABLEMAXSIZE];
PCB * processTable;

// Address base for OS code in this version
int OS_address_base; // = PROCESSTABLEMAXSIZE * MAINMEMORYSECTIONSIZE;

// Identifier of the current executing process
int executingProcessID=NOPROCESS;

// Identifier of the System Idle Process
int sipID;

// Initial PID for assignation (Not assigned)
int initialPID=-1;

// Begin indes for daemons in programList
// int baseDaemonsInProgramList; 

// Array that contains the identifiers of the READY processes
// heapItem readyToRunQueue[NUMBEROFQUEUES][PROCESSTABLEMAXSIZE];
heapItem *readyToRunQueue[NUMBEROFQUEUES];
int numberOfReadyToRunProcesses[NUMBEROFQUEUES]={0,0};
char * queueNames [NUMBEROFQUEUES]={"USER","DAEMONS"}; 

// Variable containing the number of not terminated user processes
int numberOfNotTerminatedUserProcesses=0;

// char DAEMONS_PROGRAMS_FILE[MAXFILENAMELENGTH]="teachersDaemons";

int MAINMEMORYSECTIONSIZE = 60;

extern int MAINMEMORYSIZE;

int PROCESSTABLEMAXSIZE = 4;

// Initial set of tasks of the OS
void OperatingSystem_Initialize(int programsFromFileIndex) {
	
	int i, selectedProcess;
	FILE *programFile; // For load Operating System Code
	
// In this version, with original configuration of memory size (300) and number of processes (4)
// every process occupies a 60 positions main memory chunk 
// and OS code and the system stack occupies 60 positions 

	MAINMEMORYSECTIONSIZE =(MAINMEMORYSIZE / (PROCESSTABLEMAXSIZE+1));
	OS_address_base = PROCESSTABLEMAXSIZE * MAINMEMORYSECTIONSIZE;

	if (initialPID<0) // if not assigned in options...
		initialPID=PROCESSTABLEMAXSIZE-1; 
	
	// Space for the processTable
	processTable = (PCB *) malloc(PROCESSTABLEMAXSIZE*sizeof(PCB));
	
	// Space for the ready to run queues (one queue initially...)
	readyToRunQueue[0] = (heapItem *) malloc(PROCESSTABLEMAXSIZE*sizeof(heapItem));
	readyToRunQueue[1] = (heapItem *) malloc(PROCESSTABLEMAXSIZE*sizeof(heapItem));

	sleepingProcessesQueue=(heapItem *) malloc(PROCESSTABLEMAXSIZE*sizeof(heapItem)); //Ej 7 V2

	programFile=fopen("OperatingSystemCode", "r");
	if (programFile==NULL){
		// Show red message "FATAL ERROR: Missing Operating System!\n"
		ComputerSystem_DebugMessage(99,SHUTDOWN,"FATAL ERROR: Missing Operating System!\n");
		exit(1);		
	}

	// Obtain the memory requirements of the program
	int processSize=OperatingSystem_ObtainProgramSize(programFile);

	// Load Operating System Code
	OperatingSystem_LoadProgram(programFile, OS_address_base, processSize);
	
	// Process table initialization (all entries are free)
	for (i=0; i<PROCESSTABLEMAXSIZE;i++){
		processTable[i].busy=0;
	}
	// Initialization of the interrupt vector table of the processor
	Processor_InitializeInterruptVectorTable(OS_address_base+2);
		
	// Include in program list all user or system daemon processes
	OperatingSystem_PrepareDaemons(programsFromFileIndex);
	
	// Create all user processes from the information given in the command line

	ComputerSystem_FillInArrivalTimeQueue(); //V3 Ejercicio 1 c
	OperatingSystem_PrintStatus();//V3 Ejercicio 1 d
	if(OperatingSystem_LongTermScheduler()==0){
		// Simulation must finish, telling sipID to finish
		OperatingSystem_ReadyToShutdown();
		if (executingProcessID==sipID && numberOfProgramsInArrivalTimeQueue==0) {
			// finishing sipID, change PC to address of OS HALT instruction
			Processor_CopyInSystemStack(MAINMEMORYSIZE-1,OS_address_base+1);
			executingProcessID=NOPROCESS;
			OperatingSystem_ShowTime(SHUTDOWN);
			ComputerSystem_DebugMessage(99,SHUTDOWN,"The system will shut down now...\n");
			return;
		}
	}
	OperatingSystem_PrintStatus();
	
	if (strcmp(programList[processTable[sipID].programListIndex]->executableName,"SystemIdleProcess")
		&& processTable[sipID].state==READY) {
		// Show red message "FATAL ERROR: Missing SIP program!\n"
		ComputerSystem_DebugMessage(99,SHUTDOWN,"FATAL ERROR: Missing SIP program!\n");
		exit(1);		
	}

	// At least, one process has been created
	// Select the first process that is going to use the processor
	selectedProcess=OperatingSystem_ShortTermScheduler();

	// Assign the processor to the selected process
	OperatingSystem_Dispatch(selectedProcess);

	// Initial operation for Operating System
	Processor_SetPC(OS_address_base);
}

// The LTS is responsible of the admission of new processes in the system.
// Initially, it creates a process from each program specified in the 
// 			command line and daemons programs
int OperatingSystem_LongTermScheduler() {
  
	int PID, i,
		numberOfSuccessfullyCreatedProcesses=0;
	
	 
	while(OperatingSystem_IsThereANewProgram()==YES){
		i=Heap_poll(arrivalTimeQueue,QUEUE_ARRIVAL,&numberOfProgramsInArrivalTimeQueue);
		PID=OperatingSystem_CreateProcess(i);
		if(PID==NOFREEENTRY){
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(103,ERROR,programList[i]->executableName);
		}else if(PID==PROGRAMDOESNOTEXIST){
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(104,ERROR,programList[i]->executableName,"it does not exist");
		}else if(PID==PROGRAMNOTVALID){
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(104,ERROR,programList[i]->executableName,"invalid priority or size");
		}else if(PID==TOOBIGPROCESS){
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(105,ERROR,programList[i]->executableName);
		}
		else{
			numberOfSuccessfullyCreatedProcesses++;
			if (programList[i]->type==USERPROGRAM) {
				numberOfNotTerminatedUserProcesses++;
			}
			// Move process to the ready state
			OperatingSystem_MoveToTheREADYState(PID);
		}
	}

	// Return the number of succesfully created processes
	return numberOfSuccessfullyCreatedProcesses;
}

// This function creates a process from an executable program
int OperatingSystem_CreateProcess(int indexOfExecutableProgram) {
  
	int PID;
	int processSize;
	int loadingPhysicalAddress;
	int priority;
	FILE *programFile;
	PROGRAMS_DATA *executableProgram=programList[indexOfExecutableProgram];

	// Obtain a process ID
	PID=OperatingSystem_ObtainAnEntryInTheProcessTable();
	if(PID==NOFREEENTRY){
	 	return NOFREEENTRY;
	}
	// Check if programFile exists
	programFile=fopen(executableProgram->executableName, "r");

	if(programFile==NULL){
		return PROGRAMDOESNOTEXIST;
	}
	// Obtain the memory requirements of the program
	processSize=OperatingSystem_ObtainProgramSize(programFile);	

	// Obtain the priority for the process
	priority=OperatingSystem_ObtainPriority(programFile);
	
	if(priority==PROGRAMNOTVALID || processSize==PROGRAMNOTVALID){
		return PROGRAMNOTVALID;
	}
	// Obtain enough memory space
 	loadingPhysicalAddress=OperatingSystem_ObtainMainMemory(processSize, PID);

	if(loadingPhysicalAddress==TOOBIGPROCESS){
		return TOOBIGPROCESS;
	}
	// Load program in the allocated memory
	if(OperatingSystem_LoadProgram(programFile, loadingPhysicalAddress, processSize)==TOOBIGPROCESS){
		return TOOBIGPROCESS;
	}
	
	// PCB initialization
	OperatingSystem_PCBInitialization(PID, loadingPhysicalAddress, processSize, priority, indexOfExecutableProgram);
	
	// Show message "Process [PID] created from program [executableName]\n"
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(112,SYSPROC,PID,statesNames[NEW], executableProgram->executableName);
	//OperatingSystem_ShowTime(SYSPROC);
	//ComputerSystem_DebugMessage(70,SYSPROC,PID, executableProgram->executableName);
	
	return PID;
}


// Main memory is assigned in chunks. All chunks are the same size. A process
// always obtains the chunk whose position in memory is equal to the processor identifier
int OperatingSystem_ObtainMainMemory(int processSize, int PID) {

 	if (processSize>MAINMEMORYSECTIONSIZE)
		return TOOBIGPROCESS;
	
 	return PID*MAINMEMORYSECTIONSIZE;
}


// Assign initial values to all fields inside the PCB
void OperatingSystem_PCBInitialization(int PID, int initialPhysicalAddress, int processSize, int priority, int processPLIndex) {

	processTable[PID].busy=1;
	processTable[PID].initialPhysicalAddress=initialPhysicalAddress;
	processTable[PID].processSize=processSize;
	processTable[PID].copyOfSPRegister=initialPhysicalAddress+processSize;
	processTable[PID].state=NEW;
	processTable[PID].priority=priority;
	processTable[PID].programListIndex=processPLIndex;
	// Daemons run in protected mode and MMU use real address
	if (programList[processPLIndex]->type == DAEMONPROGRAM) {
		processTable[PID].queueID=DAEMONSQUEUE;
		processTable[PID].copyOfPCRegister=initialPhysicalAddress;
		processTable[PID].copyOfPSWRegister= ((unsigned int) 1) << EXECUTION_MODE_BIT;
	} 
	else {
		processTable[PID].queueID=USERPROCESSQUEUE;
		processTable[PID].copyOfPCRegister=0;
		processTable[PID].copyOfPSWRegister=0;
	}
	processTable[PID].copyOfARegister=0;
	processTable[PID].copyOfBRegister=0;
	processTable[PID].copyOfAccumulatorRegister=0;
	processTable[PID].whenToWakeUp=0;

}


// Move a process to the READY state: it will be inserted, depending on its priority, in
// a queue of identifiers of READY processes
void OperatingSystem_MoveToTheREADYState(int PID) {

	int queue=processTable[PID].queueID;
	if (Heap_add(PID, readyToRunQueue[queue],QUEUE_PRIORITY ,&(numberOfReadyToRunProcesses[queue]) ,PROCESSTABLEMAXSIZE)>=0) {
		OperatingSystem_ShowTime(SYSPROC);
		ComputerSystem_DebugMessage(110,SYSPROC,PID,programList[processTable[PID].programListIndex]->executableName,statesNames[processTable[PID].state],statesNames[READY]);
		processTable[PID].state=READY;
	}
	//OperatingSystem_PrintReadyToRunQueue();
}


// The STS is responsible of deciding which process to execute when specific events occur.
// It uses processes priorities to make the decission. Given that the READY queue is ordered
// depending on processes priority, the STS just selects the process in front of the READY queue
int OperatingSystem_ShortTermScheduler() {
	
	int selectedProcess;

	selectedProcess=OperatingSystem_ExtractFromReadyToRun();
	
	
	return selectedProcess;
}


// Return PID of more priority process in the READY queue
int OperatingSystem_ExtractFromReadyToRun() {
  
	int selectedProcess=NOPROCESS;

	for(int i=0;i<NUMBEROFQUEUES;i++){
		if((selectedProcess=Heap_poll(readyToRunQueue[i],QUEUE_PRIORITY ,&(numberOfReadyToRunProcesses[i]))) != NOPROCESS){
			break;
		}
	}
	// Return most priority process or NOPROCESS if empty queue
	return selectedProcess; 
}


// Function that assigns the processor to a process
void OperatingSystem_Dispatch(int PID) {
	OperatingSystem_ShowTime(SHORTTERMSCHEDULE);
	ComputerSystem_DebugMessage(110,SYSPROC,PID,programList[processTable[PID].programListIndex]->executableName,statesNames[READY],statesNames[EXECUTING]);			
	// The process identified by PID becomes the current executing process
	executingProcessID=PID;
	// Change the process' state
	processTable[PID].state=EXECUTING;
	// Modify hardware registers with appropriate values for the process identified by PID
	OperatingSystem_RestoreContext(PID);
}


// Modify hardware registers with appropriate values for the process identified by PID
void OperatingSystem_RestoreContext(int PID) {
  
	// New values for the CPU registers are obtained from the PCB
	Processor_CopyInSystemStack(MAINMEMORYSIZE-1,processTable[PID].copyOfPCRegister);
	Processor_CopyInSystemStack(MAINMEMORYSIZE-2,processTable[PID].copyOfPSWRegister);
	Processor_SetRegisterSP(processTable[PID].copyOfSPRegister);
	Processor_SetRegisterA(processTable[PID].copyOfARegister);
	Processor_SetRegisterB(processTable[PID].copyOfBRegister);
	Processor_SetAccumulator(processTable[PID].copyOfAccumulatorRegister);
	// Same thing for the MMU registers
	MMU_SetBase(processTable[PID].initialPhysicalAddress);
	MMU_SetLimit(processTable[PID].processSize);
}


// Function invoked when the executing process leaves the CPU 
void OperatingSystem_PreemptRunningProcess() {

	// Save in the process' PCB essential values stored in hardware registers and the system stack
	OperatingSystem_SaveContext(executingProcessID);
	// Change the process' state
	OperatingSystem_MoveToTheREADYState(executingProcessID);
	// The processor is not assigned until the OS selects another process
	executingProcessID=NOPROCESS;
}


// Save in the process' PCB essential values stored in hardware registers and the system stack
void OperatingSystem_SaveContext(int PID) {
	
	// Load PC saved for interrupt manager
	processTable[PID].copyOfPCRegister=Processor_CopyFromSystemStack(MAINMEMORYSIZE-1);
	
	// Load PSW saved for interrupt manager
	processTable[PID].copyOfPSWRegister=Processor_CopyFromSystemStack(MAINMEMORYSIZE-2);
	
	processTable[PID].copyOfSPRegister=Processor_GetRegisterSP();

	processTable[PID].copyOfARegister=Processor_GetRegisterA();
	processTable[PID].copyOfBRegister=Processor_GetRegisterB();
	processTable[PID].copyOfAccumulatorRegister=Processor_GetAccumulator();
}


// Exception management routine
void OperatingSystem_HandleException() {
  
	// Show message "Process [executingProcessID] has generated an exception and is terminating\n"
	OperatingSystem_ShowTime(INTERRUPT);
	ComputerSystem_DebugMessage(71,SYSPROC,executingProcessID,programList[processTable[executingProcessID].programListIndex]->executableName);
	
	OperatingSystem_TerminateExecutingProcess();
	OperatingSystem_PrintStatus();
}

// All tasks regarding the removal of the executing process
void OperatingSystem_TerminateExecutingProcess() {
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(110,SYSPROC,executingProcessID,programList[processTable[executingProcessID].programListIndex]->executableName,statesNames[processTable[executingProcessID].state],statesNames[EXIT]);
	processTable[executingProcessID].state=EXIT;
	if (programList[processTable[executingProcessID].programListIndex]->type==USERPROGRAM) {
		// One more user process that has terminated
		numberOfNotTerminatedUserProcesses--;
	}
	if (numberOfNotTerminatedUserProcesses==0 && numberOfProgramsInArrivalTimeQueue==0) {
		// Simulation must finish, telling sipID to finish
		OperatingSystem_ReadyToShutdown();
		if (executingProcessID==sipID) {
			// finishing sipID, change PC to address of OS HALT instruction
			Processor_CopyInSystemStack(MAINMEMORYSIZE-1,OS_address_base+1);
			executingProcessID=NOPROCESS;
			OperatingSystem_ShowTime(SHUTDOWN);
			ComputerSystem_DebugMessage(99,SHUTDOWN,"The system will shut down now...\n");
			return; // Don't dispatch any process
		}
		
	}
	// Select the next process to execute (sipID if no more user processes)
	int selectedProcess=OperatingSystem_ShortTermScheduler();

	// Assign the processor to that process
	OperatingSystem_Dispatch(selectedProcess);
}

// System call management routine
void OperatingSystem_HandleSystemCall() 	{
  
	int systemCallID;
	int selectedProcess;
	int operand2;
	
	// Register A contains the identifier of the issued system call
	systemCallID=Processor_GetRegisterC();

	operand2=Processor_GetRegisterD();
	
	switch (systemCallID) {
		case SYSCALL_PRINTEXECPID:
			OperatingSystem_ShowTime(INTERRUPT);
			// Show message: "Process [executingProcessID] has the processor assigned\n"
			ComputerSystem_DebugMessage(72,SYSPROC,executingProcessID,programList[processTable[executingProcessID].programListIndex]->executableName,Processor_GetRegisterA(),Processor_GetRegisterB());
			break;

		case SYSCALL_END:
			// Show message: "Process [executingProcessID] has requested to terminate\n"
			OperatingSystem_ShowTime(INTERRUPT);
			ComputerSystem_DebugMessage(73,SYSPROC,executingProcessID,programList[processTable[executingProcessID].programListIndex]->executableName);
			OperatingSystem_TerminateExecutingProcess();
			OperatingSystem_PrintStatus();
			
			break;
		case SYSCALL_YIELD:
			
			selectedProcess=OperatingSystem_GetAndNotExtractMostPriorityReadyToRunProcess();
			if(selectedProcess!= NOPROCESS && processTable[selectedProcess].priority==processTable[executingProcessID].priority){
				OperatingSystem_ShowTime(SHORTTERMSCHEDULE);
				ComputerSystem_DebugMessage(116, SHORTTERMSCHEDULE, executingProcessID, programList[processTable[executingProcessID].programListIndex]->executableName,
				selectedProcess, programList[processTable[selectedProcess].programListIndex]->executableName);
				selectedProcess=OperatingSystem_ShortTermScheduler();
				OperatingSystem_PreemptRunningProcess();
				OperatingSystem_Dispatch(selectedProcess);
				OperatingSystem_PrintStatus();
			}
			else{
				/*Se sigue manteniendo este mensaje porque en la V2 no se indica que haya que borrarlo*/
				OperatingSystem_ShowTime(INTERRUPT);
				ComputerSystem_DebugMessage(117, SHORTTERMSCHEDULE, executingProcessID, programList[processTable[executingProcessID].programListIndex]->executableName);
			}
			break;
		case SYSCALL_SLEEP:
			processTable[executingProcessID].whenToWakeUp=operand2>0?operand2+numberOfClockInterrupts+1:abs(Processor_GetAccumulator())+numberOfClockInterrupts+1;;
			OperatingSystem_MoveToBlockedState(executingProcessID);
			OperatingSystem_Dispatch(OperatingSystem_ShortTermScheduler());
			OperatingSystem_PrintStatus();
			break;


	}
}

void OperatingSystem_HandleClockInterrupt(){ 
	
	numberOfClockInterrupts++;
	OperatingSystem_ShowTime(INTERRUPT);
	ComputerSystem_DebugMessage(120,INTERRUPT,numberOfClockInterrupts);
	
	int unlock=0;
	
	for(int i=0;i<numberOfSleepingProcesses;i++){
		int PID=sleepingProcessesQueue[0].info;
		if(processTable[PID].whenToWakeUp == numberOfClockInterrupts){
			PID=Heap_poll(sleepingProcessesQueue,QUEUE_WAKEUP ,&(numberOfSleepingProcesses));
			OperatingSystem_MoveToTheREADYState(PID);
			unlock++;
			i--;
		}
		
	}
	OperatingSystem_LongTermScheduler(); //V3 Ej 4
	if (numberOfNotTerminatedUserProcesses == 0) //V3 Ej 5
	{
		if (OperatingSystem_IsThereANewProgram() == NOPROCESS)
		{
			OperatingSystem_ReadyToShutdown();
		}
	}
	if(unlock>0){	
		OperatingSystem_PrintStatus();
	}
	
	OperatingSystem_ChangeProcessToMostPriorityProcess();
	
 } 
	
//	Implement interrupt logic calling appropriate interrupt handle
void OperatingSystem_InterruptLogic(int entryPoint){
	
	switch (entryPoint){
		case SYSCALL_BIT: // SYSCALL_BIT=2
			OperatingSystem_HandleSystemCall();
			break;
		case EXCEPTION_BIT: // EXCEPTION_BIT=6
			OperatingSystem_HandleException();
			break;
		case CLOCKINT_BIT:
			OperatingSystem_HandleClockInterrupt();
			break;
	}

}

void OperatingSystem_PrintReadyToRunQueue(){
	OperatingSystem_ShowTime(SHORTTERMSCHEDULE);
	ComputerSystem_DebugMessage(106,SHORTTERMSCHEDULE);
	ComputerSystem_DebugMessage(113,SHORTTERMSCHEDULE);
	
	heapItem hi;
	int PID;
	int priority;
	for(int i=0;i<numberOfReadyToRunProcesses[USERPROCESSQUEUE]-1;i++){
		hi=readyToRunQueue[USERPROCESSQUEUE][i];
		PID=hi.info;
		priority=processTable[PID].priority;
		ComputerSystem_DebugMessage(107,SHORTTERMSCHEDULE,PID,priority);
		
	}
	if(numberOfReadyToRunProcesses[USERPROCESSQUEUE]>0){
		hi=readyToRunQueue[USERPROCESSQUEUE][numberOfReadyToRunProcesses[USERPROCESSQUEUE]-1];
		PID=hi.info;
		priority=processTable[PID].priority;
		ComputerSystem_DebugMessage(108,SHORTTERMSCHEDULE,PID,priority);
	}else{
		ComputerSystem_DebugMessage(109,SHORTTERMSCHEDULE);
	}

	ComputerSystem_DebugMessage(114,SHORTTERMSCHEDULE);
	for(int i=0;i<numberOfReadyToRunProcesses[DAEMONSQUEUE]-1;i++){
		hi=readyToRunQueue[DAEMONSQUEUE][i];
		PID=hi.info;
		priority=processTable[PID].priority;
		ComputerSystem_DebugMessage(107,SHORTTERMSCHEDULE,PID,priority);
		
	}
	if(numberOfReadyToRunProcesses[DAEMONSQUEUE]>0){
		hi=readyToRunQueue[DAEMONSQUEUE][numberOfReadyToRunProcesses[DAEMONSQUEUE]-1];
		PID=hi.info;
		priority=processTable[PID].priority;
		ComputerSystem_DebugMessage(108,SHORTTERMSCHEDULE,PID,priority);
	}else{
		ComputerSystem_DebugMessage(109,SHORTTERMSCHEDULE);
	}
}

void OperatingSystem_MoveToBlockedState(int PID){
	if (Heap_add(PID, sleepingProcessesQueue, QUEUE_WAKEUP,&numberOfSleepingProcesses ,PROCESSTABLEMAXSIZE)>=0) {
		OperatingSystem_ShowTime(SYSPROC); 
		ComputerSystem_DebugMessage(110,SYSPROC,PID,programList[processTable[PID].programListIndex]->executableName,statesNames[processTable[PID].state],statesNames[BLOCKED]);
		processTable[PID].state=BLOCKED;
		OperatingSystem_SaveContext(PID);
	}
}

void OperatingSystem_ChangeProcess(int PID){
	OperatingSystem_ShowTime(SHORTTERMSCHEDULE);
	ComputerSystem_DebugMessage(121, SHORTTERMSCHEDULE, executingProcessID, programList[processTable[executingProcessID].programListIndex]->executableName, PID, programList[processTable[PID].programListIndex]->executableName);
	OperatingSystem_PreemptRunningProcess();
	OperatingSystem_Dispatch(OperatingSystem_ShortTermScheduler());
	OperatingSystem_PrintStatus();
}

void OperatingSystem_ChangeProcessToMostPriorityProcess(){
	if(numberOfReadyToRunProcesses[USERPROCESSQUEUE]>0){
		int PIDMostPriorityProcessInQueue=readyToRunQueue[USERPROCESSQUEUE][0].info;
		if ((processTable[executingProcessID].priority > processTable[PIDMostPriorityProcessInQueue].priority && processTable[executingProcessID].queueID==processTable[PIDMostPriorityProcessInQueue].queueID) || processTable[executingProcessID].queueID>processTable[PIDMostPriorityProcessInQueue].queueID){
			//Heap_poll(readyToRunQueue[USERPROCESSQUEUE],QUEUE_PRIORITY ,&(numberOfReadyToRunProcesses[USERPROCESSQUEUE]));
			OperatingSystem_ChangeProcess(PIDMostPriorityProcessInQueue);
		}
	}
	else if(numberOfReadyToRunProcesses[DAEMONSQUEUE]>0){
		int PIDMostPriorityProcessInQueue=readyToRunQueue[DAEMONSQUEUE][0].info;
		if (processTable[executingProcessID].priority > processTable[PIDMostPriorityProcessInQueue].priority && processTable[executingProcessID].queueID==processTable[PIDMostPriorityProcessInQueue].queueID){
			//Heap_poll(readyToRunQueue[DAEMONSQUEUE],QUEUE_PRIORITY ,&(numberOfReadyToRunProcesses[DAEMONSQUEUE]));
			OperatingSystem_ChangeProcess(PIDMostPriorityProcessInQueue);
		}
	}
}
int OperatingSystem_GetAndNotExtractMostPriorityReadyToRunProcess() {
  
	int selectedProcess=NOPROCESS;

	for(int i=0;i<NUMBEROFQUEUES;i++){
		if(numberOfReadyToRunProcesses[i]>0){
			selectedProcess=readyToRunQueue[i][0].info;
			break;
		}
	}
	// Return most priority process or NOPROCESS if empty queue
	return selectedProcess; 
}

int OperatingSystem_GetExecutingProcessID() {
	return executingProcessID;
}