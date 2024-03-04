Ejercicio 0:
	En este caso era añadir la instrucción MEMADD, pero esta vez incorporando la Main Memory Unit
Ejercicio 1:
	La función creada es la siguiente:

	void ComputerSystem_PrintProgramList(){
		ComputerSystem_DebugMessage(101,INIT);
		for(int i=1;i<PROGRAMSMAXNUMBER;i++){
			if(programList[i]==NULL) break;
			ComputerSystem_DebugMessage(102,INIT,programList[i]->executableName,programList[i]->arrivalTime);
		}
	}	
Ejercicio 2:
	En este caso había que añadir la función anterior justo antes de la llamada a la función:
		OperatingSystem_Initialize(programsFromFilesBaseIndex);
Ejercicio 3:
	Respuesta a la pregunta: No se ejecuta dos veces porque se usa la instrucción HALT,
	 que detiene definitivamente el funcionamiento del procesador
	Para que se ejecute dos veces hay que cambiar la instrucción HALT por la instrucción TRAP 3
Ejercicio 4:
	Respuesta a la pregunta: No se ejecuta ningún programa, 
	 sino que empieza el ciclo de instrucciones del procesador (bucle infinito)
Ejercicio 6:
	Respuesta a la pregunta:
	 Con 65 se produce un error porque supera el tamaño máximo de la sección de la memoria principal.
	 Sin embargo, 50 no supera el tamaño máximo, ya que este es 60.

El ejercicio 4, 5, 6 y 7 se realizan modificando el código de dos funciones:
	OperatingSystem_LongTermScheduler() y OperatingSystem_CreateProcess()

Ejercicio 8:
	La opción --initialPID=3 cambia los PIDs de los procesos para que empiecen en el valor 3 en vez de en 0

	if (initialPID<0) // if not assigned in options...
		initialPID=PROCESSTABLEMAXSIZE-1; 

Ejercicio 9:
	void OperatingSystem_PrintReadyToRunQueue(){
		ComputerSystem_DebugMessage(106,SHORTTERMSCHEDULE);
		for(int i=0;i<numberOfReadyToRunProcesses[0]-1;i++){
			heapItem hi=readyToRunQueue[0][i];
			int PID=hi.info;
			int priority=processTable[PID].priority;
			ComputerSystem_DebugMessage(107,SHORTTERMSCHEDULE,PID,priority);
			
		}
		heapItem hi=readyToRunQueue[0][numberOfReadyToRunProcesses[0]-1];
		int PID=hi.info;
		int priority=processTable[PID].priority;

		ComputerSystem_DebugMessage(108,SHORTTERMSCHEDULE,PID,priority);
	}

Ejercicio 10:

	Se añadió el mensaje de cambio de estado en OperatingSystem_Dispatch() y OperatingSystem_MoveToTheREADYState(int PID).
	El mensaje de creación se cambió por el que había anteriormente.

Ejercicio 11:
	En que este ejercicio se han modificado las siguientes funciones:
		OperatingSystem_PrintReadyToRunQueue(): de forma que se impriman los procesos de las dos colas con el formato indicado.
		OperatingSystem_PCBInitialization(): para añadir el campo queueID e inicializarlo.
		OperatingSystem_MoveToTheREADYState(): se cambió para mover al estado listo de la cola que corresponda según el PID
		OperatingSystem_ExtractFromReadyToRun(): para que seleccione de ambas colas el proceso según la prioridad de cada una.

Ejercicio 12:
	En este caso se modificó la función OperatingSystem_HandleSystemCall() añadiendo otra funcionalidad en caso de que se produjera la llamada SYSCALL_YIELD.
	En la función se extrae de OperatingSystem_ShortTermScheduler() el proceso de mayor prioridad, y en caso de que la prioridad de ese proceso coincida con el proceso actual, se muestra el 
	mensaje indicando que se va a ceder el control del porcesador, se quita este de el estado ejecutando y se guardan los valores del pcb y se vuelve a añadir a la cola de listos 
	(todo esto con la función OperatingSystem_PreemptRunningProcess()).
	Posteriormente, se pone en el estado ejecutando al nuevo proceso con la función OperatingSystem_Dispatch().
	En caso de que la prioridad de ambos procesos no coincida se muestra un mensaje por pantalla que indica que no se cede el procesador a ningún proceso.

Ejercicio 13:
	a. ¿Por qué hace falta salvar el valor actual del registro PC del procesador y de la PSW?
		Porque de esta forma se puede detener el proceso y más adelante continuarlo desde el estado en el que se encontraba.

	b. ¿Sería necesario salvar algún valor más?
		En el PCB también se tienen que guardar el resto de registros del procesador en el momento en el que se detiene el proceso. También se pueden guardar 
		otros recursos (punteros a memoria) e información de control (prioridad, estado...).

	c. A la vista de lo contestado en los apartados a) y b) anteriores, ¿sería necesario realizar
	alguna modificación en la función OperatingSystem_RestoreContext()? ¿Por qué?
		Sería necesario guardar los registros del procesador en el estado en el que el proceso abandona la CPU, ya que en caso contrario prodrían producirse errores al reestablecerlo.
		En esa función hay que añadir la funcionalidad para recuperar el valor de los registros del procesador en el momento en el que el proceso abandona la CPU.

	d. ¿Afectarían los cambios anteriores a la implementación de alguna otra función o a la definición de alguna estructura de datos?
		Sí. Hay que añadir tres nuevos atributos al struct PCB: copyOfARegister, copyOfBRegister, copyOfAccumulatorRegister.
		También hay que inicializarlos a 0 en OperatingSystem_PCBInitialization() y modificar OperatingSystem_SaveContext() y OperatingSystem_RestoreContext() añadiendo la guardado y la
		recuperación de esos registros.

	e. A la vista de tus respuestas a las preguntas anteriores, realiza las modificaciones oportunas
	en el simulador.


Ejercicio 14 y 15:
	En este ejercicio era suficiente con añadir el siguiente código a la función OperatingSystem_Initialize():
		if(OperatingSystem_LongTermScheduler()==0){
			// Simulation must finish, telling sipID to finish
			OperatingSystem_ReadyToShutdown();
			if (executingProcessID==sipID) {
				// finishing sipID, change PC to address of OS HALT instruction
				Processor_CopyInSystemStack(MAINMEMORYSIZE-1,OS_address_base+1);
				executingProcessID=NOPROCESS;
				ComputerSystem_DebugMessage(99,SHUTDOWN,"The system will shut down now...\n");
				return;
			}
		}

Ejercicio 16:
	En este ejercicio solo hay que añadir un condicional a las instrucciones dadas para comprobar que se ejecuta en
	modo privilegiado (fichero Processor.c). En caso contrario se lanza una excepción.

	En la función OperatingSystem_PCBInitialization() se puede ver como solo se ejecutan en modo protegido los daemons del sistema,
	mientras que los procesos de usuario se ejecutan en modo usuario.

	¿En qué momento se activa y desactiva el modo protegido? 	
		Se activa cuando se produce una interrupción en la función Processor_ManageInterrupts().
		Una vez que se termina de ejecutar la interrupción, si se vuelve a ejecutar un proceso de usuario, como este se ejecuta
		en modo usuario, el modo protegido termina.
