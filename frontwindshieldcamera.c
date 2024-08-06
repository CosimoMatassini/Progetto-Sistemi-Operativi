#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	char buffer [255] = {0}; //buffer per leggere dalla pipe
	int c; //contatore per il buffer
	int n; //valore per controllare numero di byte letti dalla pipe
	int fd_datafrontcamera = open("../data/frontCamera.data", O_RDONLY); //apre il file di dati da cui leggere
	int fd_ecu = open("../pipes_and_sockets/pipe_frontwindshieldcamera", O_WRONLY); //apre la pipe in scrittura
	int fd_cameralog = open("../logs/camera.log", O_WRONLY | O_CREAT | O_TRUNC, 0644); //apre il file (e lo crea se non esiste) di log troncandolo
	do { //legge dal file di dati e scrive nella pipe di ecu e nel file di log
		c = -1;
		do { // legge un carattere alla volta fino alla fine della riga (\n o EOF)
			c++;
			n = read (fd_datafrontcamera, &buffer[c], 1); 	//legge un carattere dal file di dati
		} while (n > 0 && buffer[c] != '\n');
		buffer[c+1] = '\0';
		n = write(fd_cameralog, buffer, strlen(buffer)); //scrive nel file di log
		buffer[c] = '\0';
		n = write(fd_ecu, buffer, strlen(buffer) + 1); //scrive nella pipe di ecu
		memset(buffer, 0, sizeof(buffer)); //pulisce il buffer
		sleep(1);
	} while(n > 0); //continua finché non arriva EOF
	return 0;
}
