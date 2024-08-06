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
	char buffer[9] = {0}; //buffer per leggere dalla pipe
	int fd_urandom = open(argv[1], O_RDONLY); //apre il file di dati da cui leggere (scelto in input)
	fcntl(fd_urandom, F_SETFL, fcntl(fd_urandom, F_GETFL) | O_NONBLOCK); //imposta l'apertura in modalità non bloccante
	int fd_radarlog = open("../logs/radar.log", O_WRONLY | O_CREAT | O_TRUNC, 0644); //apre il file (e lo crea se non esiste) di log troncandolo
	int fd_ecu = open("../pipes_and_sockets/pipe_forwardfacingradar", O_WRONLY); //	apre la pipe in scrittura
	while(1) {
		n = read(fd_urandom, buffer, 8); //legge dal file di dati
		if(n == 8) { //se ha letto 8 byte scrive nella pipe di ecu e nel file di log
			buffer[8] = '\0';
			write(fd_ecu, buffer, 9);
			buffer[8] = '\n';
			write(fd_radarlog, buffer, 9);
		}
		memset(buffer, 0, 9); //pulisce il buffer
		sleep(1);
	}
	return 0;
}