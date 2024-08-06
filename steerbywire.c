#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	int n; //valore per controllare numero di byte letti dalla pipe
	char buffer[256] = {0}; //buffer per leggere dalla pipe inizializzato a 0
	char str_log[30] = {0}; //stringa per scrivere nel file di log
	int fd_ecu = open("../pipes_and_sockets/pipe_steerbywire", O_RDONLY | O_NONBLOCK); //apre la pipe in lettura non bloccante
	int fd_steerlog = open("../logs/steer.log", O_CREAT | O_WRONLY | O_TRUNC, 0644); //apre il file (e lo crea se non esiste) di log troncandolo
	while(1)
	{
		n = read(fd_ecu, buffer, sizeof(buffer)); //legge dalla pipe di ecu
		char *p = buffer; //puntatore per scorrere il buffer
		if(!*p) //se il primo carattere è 0 scrive NO ACTION nel file di log e aspetta 1 secondo
		{
			write(fd_steerlog, "NO ACTION\n", 10);
			sleep(1);
		}
		while (*p)
		{
			for(int c = 0; c < 4; c++) {
				sprintf(str_log, "STO GIRANDO A %s\n", p); //crea la stringa per il file di log
				write(fd_steerlog, str_log, strlen(str_log)); //scrive nel file di log
				sleep(1);
			}
			p += strlen(p) + 1; //incrementa il puntatore per scorrere il buffer
			memset(str_log, 0, sizeof(str_log)); //azzera la stringa di log
		}
		memset(buffer, 0, sizeof(buffer)); //azzera il buffer
	}
	return 0;
}