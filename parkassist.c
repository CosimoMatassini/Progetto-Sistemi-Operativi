#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#define READ 0
#define WRITE 1

int fd_surroundviewcameras; //pid del processo surroundviewcameras

void signal_handler() { //handler del segnale SIGTERM
	kill(fd_surroundviewcameras, SIGTERM); //invio il segnale SIGTERM al processo surroundviewcameras
	exit(0);
}

int main(int argc, char *argv[])
{
	int n; //numero di byte letti da urandom
	int fd_urandom = open(argv[1], O_RDONLY | O_NONBLOCK); //file descriptor di urandom

	int fdpipe[2]; //pipe per la comunicazione tra park_assist e surroundviewcameras
	pipe(fdpipe); //creazione della pipe

	fd_surroundviewcameras = fork(); //creazione del processo surroundviewcameras
	if(fd_surroundviewcameras == 0) { //codice surround view cameras
		int fd_cameraslog = open("../logs/cameras.log", O_WRONLY | O_CREAT | O_APPEND, 0644); //file descriptor del file cameras.log
		close(fdpipe[READ]); //chiusura del lato di lettura della pipe (serve solo la scrittura)
		char buffer[10] = {0}; //buffer per la lettura di urandom
		while(1) {
			n = read(fd_urandom, buffer, 8); //lettura di 8 byte da urandom
			if(n == 8) { //se sono stati letti 8 byte invio a park_assist e sul file di log
				write(fdpipe[WRITE], buffer, 8);
				buffer[8] = '\n';
				write(fd_cameraslog, buffer, 10);
			}
			memset(buffer, 0, 10); //azzero il buffer
			sleep(1);
		}
	}
	else { //codice park_assist
		signal(SIGTERM, signal_handler); //imposto il signal handler per SIGTERM
		char buffer[16] = {0};
		int fd_ecu = open("../pipes_and_sockets/pipe_parkassist", O_WRONLY); //file descriptor della pipe per la comunicazione con ecu
		int fd_assistlog = open("../logs/assist.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
		close(fdpipe[WRITE]); //chiusura del lato di scrittura della pipe (serve solo la lettura)
		fcntl(fdpipe[READ], F_SETFL, fcntl(fdpipe[READ], F_GETFL) | O_NONBLOCK);
		for(int c = 0; c < 30; c++) { //per 30 secondi
			n = read(fd_urandom, buffer, 8); //leggo 8 byte da urandom
			buffer[n] = '\n';	//aggiungo il carattere di fine riga
			write(fd_assistlog, buffer, 10);	//scrivo sul file di log
			n += read(fdpipe[READ], &buffer[n], 8);	//leggo dalla pipe in comune con surroundviewcameras
			write(fd_ecu, buffer, n);	//scrivo su pipe_parkassist (cioè alla ecu)
			memset(buffer, 0, 16);	//azzero il buffer
			sleep(1);
		}
		kill(fd_surroundviewcameras, SIGTERM);	//invio il segnale SIGTERM al processo surroundviewcameras
	}
	return 0;
}