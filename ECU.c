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

#define READ 0
#define WRITE 1

int fd_eculog; //file descriptor per il file di log
int velocita; //velocità dell'auto
const char *sorgente_controllo; //sorgente di controllo per forwardfacingradar e frontwindshieldcamera
char buffer[1024]; //buffer per la comunicazione con i sensori e attuatori
int fd_brakebywire, fd_steerbywire, fd_throttlecontrol, fd_forwardfacingradar, fd_frontwindshieldcamera, fd_parkassist, fd_HMI; //file descriptor per i processi
int pipe_brakebywire, pipe_steerbywire, pipe_throttlecontrol, pipe_forwardfacingradar, pipe_frontwindshieldcamera, pipe_parkassist, pipe_output; //pipe per la comunicazione con i processi
int  socket_ECUandHMI[2]; //socket per la comunicazione con il processo input

int invia(int pipe, char *buffer) {
	int bufferlen = strlen(buffer); //per non ricalcolare ogni volta la lunghezza del buffer
	int n = write(pipe, buffer, (bufferlen + 1)); //scrive sulla pipe il buffer
	if(pipe != pipe_output) { //se la pipe non è quello di output allora ci scrive per non duplicare il messaggio
		n = write(pipe_output, buffer, (bufferlen + 1));
	}
	*(buffer+bufferlen) = '\n'; //aggiunge il carattere di fine riga
	*(buffer+bufferlen+1) = '\0'; //aggiunge il carattere di fine stringa
	n = write(fd_eculog, buffer, (bufferlen + 1)); //scrive sul file di log
	return n;
};

void parcheggio() {
	//vengono inviati i segnali di terminazione ai processi, chiusi i file descriptor e le pipe e rimossi i file di pipe
	
	kill(fd_steerbywire, SIGTERM);
	kill(fd_throttlecontrol, SIGTERM);
	kill(fd_forwardfacingradar, SIGTERM);
	kill(fd_frontwindshieldcamera, SIGTERM);
	close(fd_steerbywire);
	close(fd_throttlecontrol);
	close(fd_forwardfacingradar);
	close(fd_frontwindshieldcamera);
	close(pipe_steerbywire);
	close(pipe_throttlecontrol);
	close(pipe_forwardfacingradar);
	close(pipe_frontwindshieldcamera);
	unlink("../pipes_and_sockets/pipe_steerbywire");
	unlink("../pipes_and_sockets/pipe_throttlecontrol");
	unlink("../pipes_and_sockets/pipe_frontwindshieldcamera");
	unlink("../pipes_and_sockets/pipe_forwardfacingradar");
	
	char message [] = "Procedura di parcheggio iniziata\0\0";
	invia(pipe_output, message); //invia il messaggio di inizio parcheggio a output
	while(velocita > 0) {	//fino a quando la velocità non è 0
		char str_brakebywire[9] = "FRENO 5\0\0";
		invia(pipe_brakebywire, str_brakebywire); //invia il comando di freno a brakebywire
		velocita -= 5; //decrementa la velocità di 5
		sleep(1);
	}

	kill(fd_brakebywire, SIGTERM);
	close(fd_brakebywire);
	close(pipe_brakebywire);
	unlink("../pipes_and_sockets/pipe_brakebywire");
	
	unlink("../pipes_and_sockets/pipe_parkassist"); //rimuove una eventuale pipe precedente
	mkfifo("../pipes_and_sockets/pipe_parkassist", 0644); //crea la pipe per la comunicazione con parkassist
	while(1) {
		int parcheggiato = 1; //flag per verificare se il parcheggio è andato a buon fine
		fd_parkassist = fork(); //crea il processo parkassist
		if(fd_parkassist == 0) //il processo figlio esegue il codice di parkassist
			execl("../bin/parkassist","./parkassist", sorgente_controllo, NULL);

		pipe_parkassist = open("../pipes_and_sockets/pipe_parkassist", O_RDONLY); //apre la pipe per la comunicazione con parkassist

		for(int c = 0; c < 30; c++) {
			read(pipe_parkassist, buffer, sizeof(buffer)); //legge i dati da parkassist
			if(!(strstr(buffer, "172A") == NULL || strstr(buffer, "D693") == NULL || strstr(buffer, "0000") == NULL ||
				strstr(buffer, "BDD8") == NULL || strstr(buffer, "FAEE") == NULL || strstr(buffer, "4300") == NULL)) { //se il buffer contiene almeno uno di questi valori
				kill(fd_parkassist, SIGTERM); //invia il segnale di terminazione a parkassist
				parcheggiato = 0;	//il parcheggio non è andato a buon fine
				break;
			}
			memset(buffer, 0, sizeof(buffer));	//azzera il buffer
		}
		if(parcheggiato == 1) { //se il parcheggio è andato a buon fine
			char msg_success[] = "Procedura di parcheggio terminata con successo\0\0";
			invia(pipe_output, msg_success); //invia il messaggio di successo a output
			//viene inviato il segnale di terminazione ai processi, chiusi i file descriptor e le pipe e rimossi i file di pipe
			kill(fd_parkassist, SIGTERM);
			kill(fd_HMI, SIGTERM);
			close(fd_parkassist);
			close(fd_HMI);
			close(pipe_parkassist);
			close(pipe_output);
			unlink("../pipes_and_sockets/pipe_parkassist");
			unlink("../pipes_and_sockets/pipe_output");
			system("rm -rf ../pipes_and_sockets");
			exit(0); //il programma termina con successo
		}
		char msg_fail[] = "Procedura di parcheggio fallita\0\0";
		invia(pipe_output, msg_fail); //invia il messaggio di fallimento a output
	} //e ripete il ciclo fino a quando il parcheggio non è andato a buon fine
}

void ECU_sigterm_handler() { //handler di default per i segnali di terminazione
	//viene inviato il segnale di terminazione ai processi, chiusi i file descriptor e le pipe e rimossi i file di pipe
	kill(fd_brakebywire, SIGTERM);
	kill(fd_steerbywire, SIGTERM);
	kill(fd_throttlecontrol, SIGTERM);
	kill(fd_forwardfacingradar, SIGTERM);
	kill(fd_frontwindshieldcamera, SIGTERM);
	kill(fd_HMI, SIGTERM);
	kill(fd_parkassist, SIGTERM);
	close(fd_brakebywire);
	close(fd_steerbywire);
	close(fd_throttlecontrol);
	close(fd_forwardfacingradar);
	close(fd_frontwindshieldcamera);
	close(fd_HMI);
	close(fd_parkassist);
	close(socket_ECUandHMI[0]);
	close(socket_ECUandHMI[1]);
	close(pipe_brakebywire);
	close(pipe_steerbywire);
	close(pipe_throttlecontrol);
	close(pipe_forwardfacingradar);
	close(pipe_frontwindshieldcamera);
	close(pipe_output);
	close(pipe_parkassist);
	unlink("../pipes_and_sockets/pipe_brakebywire");
	unlink("../pipes_and_sockets/pipe_steerbywire");
	unlink("../pipes_and_sockets/pipe_throttlecontrol");
	unlink("../pipes_and_sockets/pipe_frontwindshieldcamera");
	unlink("../pipes_and_sockets/pipe_forwardfacingradar");
	unlink("../pipes_and_sockets/pipe_output");
	unlink("../pipes_and_sockets/pipe_parkassist");
	system("rm -rf ../pipes_and_sockets");
	exit(3);
}

void sigusr1_handler() { //handler per SIGUSR1 (segnale inviato da throttle control)
	velocita = 0; //imposta la velocità a 0
	char msg_arresto[] = "TERMINAZIONE DA ARRESTO DI THROTTLE CONTROL\0\0"; //messaggio di terminazione di throttle control
	invia(pipe_output, msg_arresto); //informa output della terminazione da throttle control
	//viene inviato il segnale di terminazione ai processi, chiusi i file descriptor e le pipe e rimossi i file di pipe
	kill(fd_brakebywire, SIGTERM);
	kill(fd_steerbywire, SIGTERM);
	kill(fd_throttlecontrol, SIGTERM);
	kill(fd_forwardfacingradar, SIGTERM);
	kill(fd_frontwindshieldcamera, SIGTERM);
	kill(fd_HMI, SIGTERM);
	close(fd_brakebywire);
	close(fd_steerbywire);
	close(fd_throttlecontrol);
	close(fd_forwardfacingradar);
	close(fd_frontwindshieldcamera);
	close(fd_HMI);
	close(pipe_output);
	close(pipe_brakebywire);
	close(pipe_steerbywire);
	close(pipe_throttlecontrol);
	close(pipe_forwardfacingradar);
	close(pipe_frontwindshieldcamera);
	unlink("../pipes_and_sockets/pipe_brakebywire");
	unlink("../pipes_and_sockets/pipe_steerbywire");
	unlink("../pipes_and_sockets/pipe_throttlecontrol");
	unlink("../pipes_and_sockets/pipe_frontwindshieldcamera");
	unlink("../pipes_and_sockets/pipe_forwardfacingradar");
	unlink("../pipes_and_sockets/pipe_output");
	system("rm -rf ../pipes_and_sockets");
	exit(4);
};

void sigusr2_handler() { //handler per SIGUSR2 (comando inviato da input)
	char buffer[15] = {0}; //buffer per leggere il comando di input
	read(socket_ECUandHMI[0], buffer, sizeof(buffer)); //legge il comando inviato dal processo input (HMI)
	if(strcmp(buffer, "ARRESTO") == 0) { //comando di arresto da parte di HMI
		//arresta brakebywire
		kill(fd_brakebywire, SIGUSR2);
		velocita = 0; 
	}
	else { //comando di parcheggio da parte di HMI
		void (*oldHandler) (int); //variabile per salvare temporaneamente l'handler
		oldHandler = signal(SIGUSR2, SIG_IGN); //ignora il segnale SIGUSR2
		parcheggio(); //esegue la procedura di parcheggio
		signal (SIGUSR2, oldHandler); //ripristina l'handler
	}
}


int main(int argc, char *argv[]) {
	//indipendentemente da dove viene eseguito il programma, la working directory viene impostata nella directory del programma
	char newWorkingDir[200] = {0};
	strncpy(newWorkingDir, argv[0], 200);
	int newStrLength = (long) strrchr(newWorkingDir, '/') - (long) &newWorkingDir;
	newWorkingDir[newStrLength] = '\0';
	if (!(strcmp(newWorkingDir, ".") == 0)) {
		chdir(newWorkingDir);
	}

	system("rm -f ../logs/*.log"); //rimuove eventuali file di log precedenti


	if (argc < 2) { //controlla il numero di argomenti
		fprintf(stderr,"Inserire il corretto numero di argomenti! Utilizzo: %s NORMALE o %s ARTIFICIALE\n", argv[0], argv[0]);
		exit(1);
	}
	// viene scelta la sorgente di controllo per forwardfacingradar e parkassist
	if(strcmp(argv[1], "ARTIFICIALE") == 0) {
		sorgente_controllo = "../data/urandomARTIFICIALE.binary";
	}
	else if (strcmp(argv[1], "NORMALE") == 0){
		sorgente_controllo = "/dev/urandom";
	} else { //se l'argomento non è corretto il programma termina con un messaggio di errore
		fprintf(stderr,"Specifiche non corrette! Inserire NORMALE o ARTIFICIALE come argomento\n");
		exit(2);
	}

	//CREAZIONE PROCESSI
	
	system("mkdir -p ../pipes_and_sockets"); //crea la cartella per le pipe e i socket

	//creazione pipe per comunicazione con HMI output
	unlink("../pipes_and_sockets/pipe_output");
	mkfifo("../pipes_and_sockets/pipe_output", 0644);
	pipe_output = open("../pipes_and_sockets/pipe_output", O_WRONLY);

	//creazione processo throttle control
	unlink("../pipes_and_sockets/pipe_throttlecontrol");
	mkfifo("../pipes_and_sockets/pipe_throttlecontrol", 0644);
	fd_throttlecontrol = fork();
	if(fd_throttlecontrol == 0)
		execl("../bin/throttlecontrol", "./throttlecontrol", NULL);

	//creazione processo brake by wire
	unlink("../pipes_and_sockets/pipe_brakebywire");
	mkfifo("../pipes_and_sockets/pipe_brakebywire", 0644);
	fd_brakebywire = fork();
	if(fd_brakebywire == 0)
		execl("../bin/brakebywire", "./brakebywire", NULL);

	//creazione processo front windshield camera
	unlink("../pipes_and_sockets/pipe_frontwindshieldcamera");
	mkfifo("../pipes_and_sockets/pipe_frontwindshieldcamera", 0644);
	fd_frontwindshieldcamera = fork();
	if(fd_frontwindshieldcamera == 0)
		execl("../bin/frontwindshieldcamera", "./frontwindshieldcamera", NULL);

	//creazione processo forwardfacingradar
	unlink("../pipes_and_sockets/pipe_forwardfacingradar");
	mkfifo("../pipes_and_sockets/pipe_forwardfacingradar", 0644);
	fd_forwardfacingradar = fork();
	if(fd_forwardfacingradar == 0)
		execl("../bin/forwardfacingradar", "./forwardfacingradar", sorgente_controllo, NULL);

	//controlla il comando di avvio dall'utente
	do {
		scanf("%s", buffer);
	} while(strcmp(buffer, "INIZIO") != 0);

	//apertura pipe per la comunicazione con i processi
	pipe_throttlecontrol = open("../pipes_and_sockets/pipe_throttlecontrol", O_WRONLY);
	pipe_brakebywire = open("../pipes_and_sockets/pipe_brakebywire", O_WRONLY);
	pipe_forwardfacingradar = open("../pipes_and_sockets/pipe_forwardfacingradar", O_RDONLY | O_NONBLOCK);
	pipe_frontwindshieldcamera = open("../pipes_and_sockets/pipe_frontwindshieldcamera", O_RDONLY);

	//creazione processo steer by wire
	unlink("../pipes_and_sockets/pipe_steerbywire");
	mkfifo("../pipes_and_sockets/pipe_steerbywire", 0644);
	fd_steerbywire = fork();
	if(fd_steerbywire == 0)
		execl("../bin/steerbywire", "./steerbywire", NULL);
	pipe_steerbywire = open("../pipes_and_sockets/pipe_steerbywire", O_WRONLY);

	socketpair(AF_UNIX, SOCK_STREAM, 0, socket_ECUandHMI); //crea il socket per la comunicazione con il processo input (HMI)

	fd_HMI = fork(); //creazione processo input
	if(fd_HMI == 0) { //codice input per la comunicazione con l'utente

		char buf[10] = {0}; //buffer per leggere il comando di input
		fcntl(socket_ECUandHMI[1], F_SETFL, (F_GETFL | O_NONBLOCK));
		while (1) {
			scanf("%s", buffer); //legge il comando di input dall'utente
			if(strcmp(buffer, "INIZIO") == 0) { //se il comando è INIZIO
				read(socket_ECUandHMI[1], buf, sizeof(buf)); //legge il comando inviatogli dal processo ECU (esso indica se la macchina è in PERICOLO o no)
				if(strcmp(buf, "PERICOLO") == 0) { //se il comando è PERICOLO
					write(socket_ECUandHMI[1], buffer, sizeof(buffer)); //invia il comando "INIZIO" al processo ECU
					memset(buf, 0, sizeof(buf)); //azzera il buffer
				}
			}
			else if(strcmp(buffer, "ARRESTO") == 0 || strcmp(buffer, "PARCHEGGIO") == 0) {	//se il comando è ARRESTO o PARCHEGGIO
				write(socket_ECUandHMI[1], buffer, strlen(buffer)); //invia il comando di appropriato al processo ECU
				kill(getppid(), SIGUSR2); //invia il segnale SIGUSR2 al processo della ECU
			}
			memset(buffer, 0, sizeof(buffer)); //azzera il buffer
		}
	}
	else { //codice central ecu per la comunicazione con tutti i sensori e attuatori
		signal(SIGUSR1, sigusr1_handler); //ridefinisce l'handler per SIGUSR1 (segnale inviato da throttle control) 
		signal(SIGUSR2, sigusr2_handler); //ridefinisce l'handler per SIGUSR2 (comando inviato da input)
		signal(SIGINT, ECU_sigterm_handler); //ridefinisce l'handler per SIGINT
		signal(SIGTERM, ECU_sigterm_handler); //ridefinisce l'handler per SIGTERM
		int n = 0; //variabile per il numero di byte letti
		velocita = 0; //inizializza la velocità a 0
		char buffer[1024]; //buffer per la comunicazione con i sensori e attuatori
		fd_eculog = open("../logs/ECU.log", O_CREAT | O_WRONLY | O_TRUNC, 0644); //apre il file di log in scrittura e lo azzera (se non esiste lo crea)
		while(1) {
			memset(buffer, 0, sizeof(buffer));	//azzera il buffer
			n = read(pipe_forwardfacingradar, buffer, sizeof(buffer)); //la central-ecu non deve fare niente con i dati di forwardfacingradar
			memset(buffer, 0, sizeof(buffer)); //azzera il buffer
			n = read(pipe_frontwindshieldcamera, buffer, sizeof(buffer)); //la central-ecu legge i dati di frontwindshieldcamera
			char *p = buffer; //puntatore per scorrere il buffer
			while (*p) { //finchè non arriva alla fine del buffer
				if(strcmp(p, "DESTRA") == 0 || strcmp(p, "SINISTRA") == 0){ //se il comando è DESTRA o SINISTRA lo inoltra a steerbywire
					char str_log[11] = {0};
					strcpy(str_log, p);
					invia(pipe_steerbywire, str_log);
				}
				else if(strcmp(p, "PERICOLO") == 0) { //se il comando è PERICOLO lo inoltra a brakebywire
					void (*oldHandler) (int); //variabile per salvare temporaneamente l'handler
					oldHandler = signal(SIGUSR2, SIG_IGN);
					kill(fd_brakebywire, SIGUSR2); //invia il segnale SIGUSR2 a brakebywire
					char str_warning[]="PERICOLO, AUTO ARRESTATA\0\0";
					invia(pipe_output, str_warning); //invia il messaggio di warning a output
					velocita = 0; //imposta la velocità a 0
					write(socket_ECUandHMI[0], "PERICOLO\0", 10); //invia il messaggio di PERICOLO a input
					do{ //aspetta che il processo di input invii INIZIO
						read(socket_ECUandHMI[0], buffer, sizeof(buffer));
					}while(strcmp(buffer, "INIZIO") != 0);
					signal (SIGUSR2, oldHandler);	//ripristina l'handler
				}
				else if(strcmp(p, "PARCHEGGIO") == 0) { //se il comando è PARCHEGGIO esegue la procedura di parcheggio
					parcheggio();
				}
				else { //se il comando è un numero
					int velocita_richiesta = atoi(p); //converte il comando in un intero
					if(velocita_richiesta > velocita) {	//se la velocità richiesta è maggiore della velocità attuale
						char str_throttlecontrol[14] = "INCREMENTO 5\0\0";
						invia(pipe_throttlecontrol, str_throttlecontrol); //invia il comando di incremento a throttlecontrol
						velocita += 5; //incrementa la velocità di 5
						sleep(1);
					}
					else if(velocita_richiesta < velocita) { //se la velocità richiesta è minore della velocità attuale
						char str_brakebywire[9] = "FRENO 5\0\0";
						invia(pipe_brakebywire, str_brakebywire); //invia il comando di decremento a brakebywire
						velocita -= 5; //decrementa la velocità di 5
						sleep(1);
					}
				}
				p += strlen(p) + 1; //incrementa il puntatore per scorrere il buffer
			}
		}
	}
	return 0;
}