#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/un.h>
#include <time.h>

int main(int argc, char *argv[])
{
	int fd_ecu; //file descriptor della pipe di ecu
	int fallimento; //variabile per simulare il fallimento
	char buffer[13] = {0}; //buffer per leggere dalla pipe
	char str_log[30] = {0};	//stringa per scrivere nel file di log
	time_t t;
	struct tm tm;
	fd_ecu = open("../pipes_and_sockets/pipe_throttlecontrol", O_RDONLY); //apre la pipe in lettura
	srand(time(NULL)); //inizializza il generatore di numeri casuali
	int fd_throttlelog = open("../logs/throttle.log", O_CREAT | O_WRONLY | O_TRUNC, 0644); //apre il file (e lo crea se non esiste) di log troncandolo
	while (1)
	{
		read(fd_ecu, buffer, sizeof(buffer)); //legge dalla pipe di ecu
		fallimento = rand() % 100000; //genera un numero a caso tra 0 e 99999
		if(fallimento == 0) { //se il numero è 0 (probabilità di 1 su 10^-5) invia un segnale SIGUSR1 al processo padre (ovvero ecu)
			kill(getppid(), SIGUSR1);
		}
		else { //altrimenti scrive nel file di log la data e l'ora correnti e l'aumento di 5 della velocità
			t = time(NULL);
			tm = *localtime(&t);
			sprintf(str_log, "%d-%02d-%02d %02d:%02d AUMENTO 5\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
			write(fd_throttlelog, str_log, strlen(str_log)); //scrive nel file di log
		}
	}
	return 0;
}