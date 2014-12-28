/*
|--------------------------------------------------------------------------
| File Monitor
|--------------------------------------------------------------------------
|
| This program accepts a time argument, a word, and a undetermined number
| of files. The files are then monitored and if that word is entered,
| an alert will be printed in the console with a timestamp.
| Usage: ./prog time word file1 file2 [...] fileN
|
*/

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <string.h>


const int PIPE_READ = 0;
const int PIPE_WRITE = 1;
const int BUFFERSIZE = 512;
const int CHECKTIMER = 5;

int RUNTIME;
int fileNumber;

pid_t processToKill;

pid_t fileMonitorProcess;
pid_t checkProcess;
pid_t grepProcess;
pid_t tailProcess;

pid_t topPID;
pid_t *fileProcessListptr;

/****
**** SIGNAL HANDLERS 
*****/
void alarm_handler(int sig){
    int i;
    // kills all group processes
    for (i = 0; i <= fileNumber; i++) {
        if( fileProcessListptr[i] != 0){
            killpg(fileProcessListptr[i], SIGTERM);
        }
    }
    // kills main process
	killpg(getpid(), SIGTERM);
}

void sigusr_handler(int sig){
    killpg(processToKill, SIGUSR1);
}

void zombieClose(int sig){
    killpg(processToKill, SIGTERM);
}

/****
-------------------------------------------
*****/


// returns a string with current date and time
char* getDateHour() {
    char *dateHour;
    char aux_str[40];

    time_t current;
    current = time(NULL);
    struct tm *time_info = localtime(&current);
    sprintf(aux_str, "%04d-%02d-%02dT%02d:%02d:%02d", time_info->tm_year+1900,time_info->tm_mon+1,time_info->tm_mday,time_info->tm_hour,
        time_info->tm_min,time_info->tm_sec);
    dateHour = malloc(strlen(aux_str)+1);
    dateHour[0]=0;
    strcat(dateHour,aux_str);
    return dateHour;
}

// function to monitor files change
void monitorFile(char * file, char* word){
    /**
             CREATE PIPES
    **/
    int pipe1[2], pipe2[2] ;
    if(pipe(pipe1)<0 ||  pipe(pipe2)<0){
        perror("Pipe Error");
        exit(1);
    }

    int readState;  // contem o numero de bits lidos pela funcao read
    char readLine[BUFFERSIZE];  // contem a linha do ficheiro, lida pela funcao read

    
    /**
             TAIL PROCESS
    **/
    if((tailProcess=fork())==0){ 
        dup2(pipe1[PIPE_WRITE], STDOUT_FILENO);
        close(pipe1[PIPE_READ]);
        execlp("tail", "tail", "-f", "-n 0", file, NULL);
    }


    /**
             GREP PROCESS
    **/
    else if(tailProcess > 0){
        if( (grepProcess=fork())==0){
            // pipe1 -> read ;;;;;; pipe2 -> write
            close(pipe1[PIPE_WRITE]);    
            dup2(pipe1[PIPE_READ], STDIN_FILENO);
            close(pipe2[PIPE_READ]);
            dup2(pipe2[PIPE_WRITE], STDOUT_FILENO);
            execlp("grep", "grep", "--line-buffered", word, NULL);
        }

        else if(grepProcess>0){    // le do pipe2 e imprime no terminal
            close(pipe2[PIPE_WRITE]);
            readState=1;
            while(readState>0){
                readState = read(pipe2[PIPE_READ],readLine,BUFFERSIZE);
                printf("%s - %s - \"%s\"\n", getDateHour(), file,readLine);
            }
        }
        // erro a criar o fork
        else{
            perror("Grep Process Creating Error");
        }
    }

    // erro a criar o fork
    else{
        perror("Tail Process Creating Error");
        exit(1);
    }
}


// funcao que verifica se o ficheiro file foi eliminado. envia sinal SIGUSR1 para o(s) processo(s) encarregues desse ficheiro
// devolve 1 caso tenha sido removido, devolve 0 caso contrario
int checkIfFileRemoved(char * file, pid_t * ptr ,int processID){
    if(fileNumber == 0){
        killpg(topPID, SIGTERM);
    }else{
        if(access(file, F_OK) != -1){
        }else{
            fileNumber--;
            printf("File %s doesnt exist or has been removed\n", file);
            processToKill=ptr[processID];
            int erro;
            if((erro=killpg(topPID, SIGUSR1))!=0){
                perror("Signal send error");
            }
            ptr[processID] = 0;
            return 1;
        }
    }
    return 0;
}

int main(int argc, char * argv[]){
    // verifica se o numero de argumentos esta correto
	if(argc < 4) {
        fprintf( stderr, "Usage: %s time word file1 file2 [...] fileN\n", argv[0]);
        exit(1);
    }

    // tempo que o processo dura
    RUNTIME=atoi(argv[1]);

    // alarme apra terminar todos os processos
    signal(SIGALRM, alarm_handler);
    signal(SIGUSR1, sigusr_handler);
    alarm(RUNTIME);
    // -------

    fileNumber = argc - 3;  // numero de ficheiros introduzidos pelo user
    pid_t fileMonitorProcessList[fileNumber];   // array com os processos filhos todos
    fileProcessListptr = fileMonitorProcessList; // apontador para o array dos processos filho
    
    int i = 0;
   // setpgrp(); // creates a group of processes
    topPID = getpid();

    while(i < fileNumber){
        // cria os processos que verificam alteracoes aos ficheiros
    	if((fileProcessListptr[i]=fork()) == 0){ 
            setpgrp();  // cria um grupo para o tail e grep
            signal(SIGUSR1, zombieClose);   // caso seja lancado o sinal SIGUSR1
            fileProcessListptr[i]=getpgrp();    // coloca os pid's dos processos num array
            monitorFile(argv[i+3], argv[2]);
    	}
        i++;
    }

    fileProcessListptr[i] = 0;  // coloca um indicador de fim do array
    
   // processo para verificar se algum ficheiro foi eliminado
    if((checkProcess = fork()) == 0)   {
        time_t current, last;
        last = time(NULL);
        do
        {
            current = time(NULL);
            if(difftime(current, last)>=5){ // se tiverem passado 5 segundos
                for (i = 0; i <= fileNumber; i++) {
                    if (fileProcessListptr[i] == 0) {   // se nao tiver pid
                        if(fileNumber==0){  // se o numero de ficheiros for 0 , termina todos os processos
                            killpg(topPID, SIGTERM);
                        }
                        continue;
                    }else{
                        if(checkIfFileRemoved(argv[i+3], fileProcessListptr, i) == 1){  // verifica se o ficheiro argv[i+3] foi eliminado. devolve 1 em caso de ter sido
                            break;
                        }
                    }
                }
                last =current;
            }
        } while (1);
    }
    

	pause();
	return 0;

}
