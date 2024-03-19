Miguel Méndez Murias
UO287687

Ejercicio 1:
    Todos los apartados hechos
Ejercicio 2:
    int OperatingSystem_GetExecutingProcessID() {
        return executingProcessID;
    }

    Mensaje añadido y cambiado por el 69:
        130, %s %d %d (PID: @G%d@@, PC: @R%d@@, Accumulator: @R%d@@, PSW: @R%x@@ [@R%s@@])\n


Ejercicio 3:
    int OperatingSystem_LongTermScheduler() {
        int PID, i,
            numberOfSuccessfullyCreatedProcesses=0;
        for (i=0; programList[i]!=NULL && i<PROGRAMSMAXNUMBER ; i++) {
            if(OperatingSystem_IsThereANewProgram()==YES){
                PID=OperatingSystem_CreateProcess(Heap_poll(arrivalTimeQueue,QUEUE_ARRIVAL,&numberOfProgramsInArrivalTimeQueue));
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
                    if (programList[i]->type==USERPROGRAM) 
                        numberOfNotTerminatedUserProcesses++;
                    // Move process to the ready state
                    OperatingSystem_MoveToTheREADYState(PID);
                }
            }
        }
        // Return the number of succesfully created processes
        return numberOfSuccessfullyCreatedProcesses;
    }


Ejercicio 4:
    void OperatingSystem_HandleClockInterrupt(){ 
        
        numberOfClockInterrupts++;
        OperatingSystem_ShowTime(INTERRUPT);
        ComputerSystem_DebugMessage(120,INTERRUPT,numberOfClockInterrupts);
        
        int unlock=0;
        
        
        for(int i=0;i<numberOfSleepingProcesses;i++){
            int PID;
            PID=sleepingProcessesQueue[i].info;
            if(processTable[PID].whenToWakeUp == numberOfClockInterrupts){
                Heap_poll(sleepingProcessesQueue,QUEUE_WAKEUP ,&(numberOfSleepingProcesses));
                OperatingSystem_MoveToTheREADYState(PID);
                unlock++;
                i--;
            }
            
        }
        OperatingSystem_LongTermScheduler();
        if(unlock>0){
            OperatingSystem_PrintStatus();
            
        }
        OperatingSystem_ChangeProcessToMostPriorityProcess();
    } 

Ejercicio 5: 
    Se modificó la función anterior

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