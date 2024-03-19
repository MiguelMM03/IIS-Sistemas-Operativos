Miguel Méndez Murias
UO287687

Ejercicio 1:
	Se ha añadido la función OperatingSystem_ShowTime() a:
		OperatingSystem_Initialize()
		OperatingSystem_CreateProcess()
		OperatingSystem_MoveToTheREADYState()
		OperatingSystem_ShortTermScheduler()
		OperatingSystem_TerminateExecutingProcess()
		OperatingSystem_InterruptLogic()
		OperatingSystem_PrintReadyToRunQueue()
	Se ha añadido la función ComputerSystem_ShowTime() a:
		ComputerSystem_PrintProgramList()
		ComputerSystem_PowerOff()
		Processor_FetchInstruction()

Ejercicio 2:
	Pasos que se hicieron:
		void Clock_Update() {
			tics++;
			// ComputerSystem_DebugMessage(97,CLOCK,tics); // Coment in V2-StudentsCode
			if(tics%intervalBetweenInterrupts==0){
				Processor_RaiseInterrupt(CLOCKINT_BIT);
			}
		}

		En el método Processor_InitializeInterruptVectorTable():
			interruptVectorTable[CLOCKINT_BIT]=interruptVectorInitialAddress+4;
		En OperatingSystemCode, se añadió lo siguiente:
			11
			IRET // Initial Operation for OS
			HALT // Shutdown the System
			// Here interrupt vector
			OS 2 // SysCall Interrupt
			IRET
			OS 6 // Exception Interrupt
			IRET
			OS 9 //Clock interrupt
			IRET


Ejercicio 3:
	Hecho
	if(!Processor_PSW_BitState(INTERRUPT_MASKED_BIT)){
		for (i=0;i<INTERRUPTTYPES;i++){
			// If an 'i'-type interrupt is pending
			if (Processor_GetInterruptLineStatus(i)) {
				// Deactivate interrupt
				Processor_ACKInterrupt(i);
				// Copy PC and PSW registers in the system stack
				Processor_CopyInSystemStack(MAINMEMORYSIZE-1, registerPC_CPU);
				Processor_CopyInSystemStack(MAINMEMORYSIZE-2, registerPSW_CPU);	
				Processor_CopyInSystemStack(MAINMEMORYSIZE - 3, registerAccumulator_CPU);
				Processor_ActivatePSW_Bit(EXECUTION_MODE_BIT);
				Processor_ActivatePSW_Bit(INTERRUPT_MASKED_BIT);
				// Activate protected excution mode
				
				// Call the appropriate OS interrupt-handling routine setting PC register
				registerPC_CPU=interruptVectorTable[i];
				break; // Don't process another interrupt
			}
		}
	}

Ejercicio 4:
	Hecho. No se puede comprobar por el problema en el ejercicio 2