#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

int fd_brakelog;

void sigusr2_handler() { //gestore del segnale SIGUSR2 che scrive nel file di log ARRESTO AUTO
	char str_arresto[] = "ARRESTO AUTO\n\0";
	write(fd_brakelog, str_arresto, strlen(str_arresto));
};

int main(int argc, char *argv[])
{
	signal(SIGUSR2, sigusr2_handler); //imposta il gestore del segnale SIGUSR2
	int n; //valore per controllare numero di byte letti dalla pipe
	char buffer[25] = {0}; //buffer per leggere dalla pipe
	int fd_ecu; //file descriptor della pipe di ecu
	char str_log[30] = {0}; //stringa per scrivere nel file di log
	time_t t;
	struct tm tm;
	fd_ecu = open("../pipes_and_sockets/pipe_brakebywire", O_RDONLY);	//apre la pipe in lettura
	fd_brakelog = open("../logs/brake.log", O_CREAT | O_WRONLY | O_TRUNC, 0644); //apre il file (e lo crea se non esiste) di log troncandolo
	while(1) //legge dalla pipe di ecu e scrive nel file di log la data e l'ora correnti e FRENO 5
	{
		n = read(fd_ecu, buffer, sizeof(buffer));
		t = time(NULL);
		tm = *localtime(&t);
		sprintf(str_log, "%d-%02d-%02d %02d:%02d FRENO 5\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
		write(fd_brakelog, str_log, strlen(str_log));
	}
	return 0;
}