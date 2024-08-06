#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
	char newWorkingDir[200] = {0};
	strncpy(newWorkingDir, argv[0], 200);
	int newStrLength = (long) strrchr(newWorkingDir, '/') - (long) &newWorkingDir;
	newWorkingDir[newStrLength] = '\0';
	if (!(strcmp(newWorkingDir, ".") == 0)) {
		chdir(newWorkingDir);
	}

	char buffer[256]; //buffer per la lettura della pipe
	int fd, n; //fd = file descriptor, n = numero di byte letti

	//apertura della pipe
	do
	{
		fd = open("../pipes_and_sockets/pipe_output", O_RDONLY);	//apre la pipe in lettura
		if (fd == -1) { //se la pipe non esiste, aspetta 1 secondo e riprova
			sleep(1);
		}
	} while (fd == -1); //

	do { //legge il contenuto della pipe e lo stampa a video
		memset(buffer, 0, sizeof(buffer)); //reimposta il buffer a 0
		n = read(fd, buffer, sizeof(buffer)); //legge il contenuto della pipe e lo mette in buffer
		char *p = buffer; //p punta al primo carattere di buffer
		while (*p) //finchè p non punta a 0
		{ 
			printf("%s\n", p); //stampa il contenuto di p
			p += strlen(p) + 1; //p punta al prossimo carattere di buffer
    	}
	} while(n != 0); //finchè non legge 0 byte

	close(fd); //chiude la pipe
	unlink("../pipes_and_sockets/pipe_output");	//elimina la pipe
	return 0;
	
}