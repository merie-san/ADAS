#include "headers.h"

int initSis(int);
int isNumber(char *);
int makePipe(void);
int closeAll(void);
void quitHandler(int);
void stopHandler(int);
void parkHandler(int);
void throttleFailureHandler(int);
int parking(void);
int ECUoutputfd, ECUinputfd, s_b_winputfd, t_cinputfd, b_b_winputfd, p_aoutputfd, debugLogfd;
int ECUlogfd, velfd;
int serverfd, clientfd, serverlen, clientlen;
int ECU_fwcSemfd, modality;
int ECUinputpid, f_w_cpid, s_b_wpid, t_cpid, b_b_wpid, p_apid;

int main(int argc, char const *argv[])
{
	char inputFromHMI[100], inputFromOt[100], wait[10];
	struct sockaddr_un severAddress;
	struct sockaddr_un clientAddress;
	struct sockaddr *serverAddrPtr;
	struct sockaddr *clientAddrPtr;

	debugLogfd = open("../log/debuglog.txt", O_CREAT | O_WRONLY | O_APPEND, 0660);
	printf("avvio ECU...\n");
	write(debugLogfd, "avvio ECU...\n", 13);
	printf("avvio ECUoutput..\n");
	write(debugLogfd, "avvio ECUoutput...\n", 19);

	sleep(1);

	if (*argv[1] == 0)
	{
		printf("ECU: esecuzione in modalità normale\n");
	}
	else
	{
		printf("ECU: esecuzione in modalità artificiale\n");
	}
	modality = *argv[1];

	// crea e apri il logfile e il file per rappresentare la velocità
	ECUlogfd = open("../log/ECU.log", O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0660);
	velfd = open("velocity.txt", O_RDWR | O_CREAT | O_TRUNC, 0660);
	write(velfd, "0", 2);

	// crea e apri i pipe per la comunicazione tra i componenti
	if (makePipe())
	{
		perror("makePipe");
		exit(EXIT_FAILURE);
	}

	// apri il pipe di input per l'ECU
	ECUinputfd = open("ECU_input", O_RDONLY);

	// creo semafori per la sincronizzazione dei processi
	unlink("ECU_SBW_semaphore");
	if (mknod("ECU_SBW_semaphore", 0010000, 0) == -1)
	{
		perror("ECU_SBW_semaphore");
		exit(EXIT_FAILURE);
	}
	chmod("ECU_SBW_semaphore", 0660);

	// inizializza i processi
	if (initSis(*argv[1]))
	{
		printf("errore inizializzazione sistema\n");
		exit(EXIT_FAILURE);
	};

	sleep(1);
	write(debugLogfd, "ECU: apertura comunicazioni con attuatori e sensori...\n", 55);

	// apro il semaforo per steer-by-wire
	ECU_fwcSemfd = open("ECU_SBW_semaphore", O_RDONLY);

	// cerco di aprire il pipe di comunicazione con steer by wire
	int temp = open("s_b_w_input", O_WRONLY);
	while (temp == -1)
	{
		temp = open("s_b_w_input", O_WRONLY);
		sleep(1);
	}
	s_b_winputfd = temp;

	// apro il pipe di comunicazione con throttle control
	t_cinputfd = open("t_c_input", O_WRONLY);

	// apro il pipe di comunicazione con brake by wire
	b_b_winputfd = open("b_b_w_input", O_WRONLY);

	// apro il pipe per la comunicazione con HMI e leggo il segnale di inizio
	readString(ECUinputfd, inputFromHMI);
	printf("ECU: ricevuto segnale \"%s\"\n", inputFromHMI);

	// creo il socket dell'ECU per comunicazione con gli altri componenti del sistema
	serverAddrPtr = (struct sockaddr *)&severAddress;
	serverlen = sizeof(severAddress);
	clientAddrPtr = (struct sockaddr *)&clientAddress;
	clientlen = sizeof(clientAddress);
	serverfd = socket(AF_UNIX, SOCK_STREAM, 0);
	severAddress.sun_family = AF_UNIX;
	strcpy(severAddress.sun_path, "ECUServer");
	unlink("ECUServer");
	bind(serverfd, serverAddrPtr, serverlen);
	listen(serverfd, 8);
	write(debugLogfd, "ECU: server creato\n", 19);

	// installo i handler
	signal(SIGTERM, quitHandler);
	signal(SIGUSR1, stopHandler);
	signal(SIGUSR2, parkHandler);
	signal(SIGQUIT, throttleFailureHandler);

	// processo figlio per ricevere gli input da HMI
	ECUinputpid = fork();
	if (ECUinputpid == 0)
	{
		execl("ECUinput", "ECUinput", &ECUinputfd, inputFromHMI, argv[1], NULL);
		perror("ECUinput");
		exit(EXIT_FAILURE);
	}

	// apertura pipe con nome per la comunicazione dei output di ECU a HMI
	do
	{
		ECUoutputfd = open("ECU_output", O_WRONLY);
		if (ECUoutputfd == -1)
		{
			sleep(1);
		}
	} while (ECUoutputfd == -1);

	// cambio il gruppo di appartenenza del processo
	setpgid(0, 0);

	// gestione concorrente delle richieste dai componenti
	while (1)
	{
		write(debugLogfd, "ECU: server in ascolto degli input dei componenti...\n", 53);
		clientfd = accept(serverfd, clientAddrPtr, &clientlen);
		if (clientfd == -1)
		{
			perror("clientfd");
			exit(EXIT_FAILURE);
		}

		write(debugLogfd, "ECU: collegamento avvenuto con successo, richiesta in elaborazione...\n", 70);
		if (fork() == 0)
		{
			char temp1[100], *temp2;
			readString(clientfd, inputFromOt);
			temp2 = inputFromOt;
			sprintf(temp1, "ECU: ricevuto messaggio \"%s\"\n", temp2);
			write(debugLogfd, temp1, strlen(temp1));

			if (isNumber(inputFromOt))
			{
				char velocityStr[100];
				lseek(velfd, 0, SEEK_SET);
				readString(velfd, velocityStr);
				temp2 = velocityStr;
				sprintf(temp1, "ECU: velocità attuale auto %s\n", temp2);
				write(debugLogfd, temp1, strlen(temp1));
				int velocity = strtol(velocityStr, NULL, 10);
				int target = strtol(inputFromOt, NULL, 10);

				if (target == velocity)
				{
					write(debugLogfd, "ECU: l'auto è già alla velocità desiderata\n", 46);
				}

				while (target < velocity)
				{
					write(ECUlogfd, "FRENO 5\n", 8);
					write(ECUoutputfd, "FRENO 5", 8);
					write(b_b_winputfd, "FRENO 5", 8);
					sleep(1);
					velocity -= 5;
					sprintf(velocityStr, "%d", velocity);
					lseek(velfd, 0, SEEK_SET);
					write(velfd, velocityStr, strlen(velocityStr) + 1);
				}

				while (target > velocity)
				{
					write(ECUlogfd, "INCREMENTO 5\n", 13);
					write(ECUoutputfd, "INCREMENTO 5", 13);
					write(t_cinputfd, "INCREMENTO 5", 13);
					sleep(1);
					velocity += 5;
					sprintf(velocityStr, "%d", velocity);
					lseek(velfd, 0, SEEK_SET);
					write(velfd, velocityStr, strlen(velocityStr) + 1);
				}
				sprintf(temp1, "ECU: velocità desiderata raggiunta. velocità=%d\n", velocity);
				write(debugLogfd, temp1, strlen(temp1));
			}
			else
			{
				if (strcmp(inputFromOt, "DESTRA") == 0)
				{
					write(ECUlogfd, "DESTRA\n", 7);
					write(ECUoutputfd, "DESTRA", 7);
					write(s_b_winputfd, "DESTRA", 7);
					write(s_b_winputfd, "DESTRA", 7);
					write(s_b_winputfd, "DESTRA", 7);
					write(s_b_winputfd, "DESTRA", 7);
					read(ECU_fwcSemfd, wait, 2);
					read(ECU_fwcSemfd, wait, 2);
					read(ECU_fwcSemfd, wait, 2);
					read(ECU_fwcSemfd, wait, 2);
				}

				if (strcmp(inputFromOt, "SINISTRA") == 0)
				{
					write(ECUlogfd, "SINISTRA\n", 9);
					write(ECUoutputfd, "SINISTRA", 9);
					write(s_b_winputfd, "SINISTRA", 9);
					write(s_b_winputfd, "SINISTRA", 9);
					write(s_b_winputfd, "SINISTRA", 9);
					write(s_b_winputfd, "SINISTRA", 9);
					read(ECU_fwcSemfd, wait, 2);
					read(ECU_fwcSemfd, wait, 2);
					read(ECU_fwcSemfd, wait, 2);
					read(ECU_fwcSemfd, wait, 2);
				}

				if (strcmp(inputFromOt, "PERICOLO") == 0)
				{
					write(ECUlogfd, "PERICOLO\n", 9);
					write(ECUoutputfd, "PERICOLO", 9);
					kill(b_b_wpid, SIGUSR1);
					lseek(velfd, 0, SEEK_SET);
					write(velfd, "0", 2);
				}

				if (strcmp(inputFromOt, "PARCHEGGIO") == 0)
				{
					int res = parking();
					while (!res)
					{
						res = parking();
						sleep(1);
					}
					kill(getppid(), SIGTERM);
				}
			}
			write(debugLogfd, "ECU: operazione richiesta completata\n", 37);
			// avverto la client del completamento della richiesta
			write(clientfd, "OK", 3);
			close(clientfd);
			exit(EXIT_SUCCESS);
		}
		close(clientfd);
		sleep(1);
	}

	return 0;
}

int initSis(int modalita)
{
	f_w_cpid = fork();
	if (f_w_cpid == 0)
	{
		execl("front_windshield_camera", "front_windshield_camera", NULL);
		return 1;
	}

	s_b_wpid = fork();
	if (s_b_wpid == 0)
	{
		execl("steer_by_wire", "steer_by_wire", NULL);
		return 1;
	}

	t_cpid = fork();
	if (t_cpid == 0)
	{
		execl("throttle_control", "throttle_control", NULL);
		return 1;
	}

	b_b_wpid = fork();
	if (b_b_wpid == 0)
	{
		execl("brake_by_wire", "brake_by_wire", NULL);
		return 1;
	}

	return 0;
}

int isNumber(char *str)
{
	int bool = 1;
	for (int i = 0; *(str + i) != '\0'; i++)
	{
		bool = bool && (isdigit(str[i]) != 0);
	}
	return bool;
}

int closeAll(void)
{
	close(ECUinputfd);
	close(ECUoutputfd);
	close(s_b_winputfd);
	close(t_cinputfd);
	close(b_b_winputfd);
	close(ECU_fwcSemfd);
	unlink("ECU_SBW_semaphore");
	unlink("ECU_input");
	unlink("ECU_output");
	unlink("s_b_w_input");
	unlink("t_c_input");
	unlink("b_b_w_input");
	write(debugLogfd, "ECU: comunicazioni chiuse\n", 26);
	close(velfd);
	unlink("velocity.txt");
	close(serverfd);
	close(clientfd);
	unlink("ECUServer");
	write(debugLogfd, "ECU: server chiuso\n", 19);
	return 0;
}

void quitHandler(int sig)
{
	closeAll();
	kill(getppid(), SIGTERM);
	kill(ECUinputpid, SIGTERM);
	kill(f_w_cpid, SIGTERM);
	kill(s_b_wpid, SIGTERM);
	kill(t_cpid, SIGTERM);
	kill(b_b_wpid, SIGTERM);
	while (wait(NULL) > 0)
	{
		;
	}
	printf("ECUoutput ha concluso la sua esecuzione\n");
	write(debugLogfd, "ECUoutput ha concluso la sua esecuzione\n", 40);
	close(debugLogfd);
	exit(EXIT_SUCCESS);
}

void stopHandler(int sig)
{
	write(ECUlogfd, "ARRESTO\n", 8);
	write(ECUoutputfd, "ARRESTO", 8);
	kill(b_b_wpid, SIGUSR1);
	lseek(velfd, 0, SEEK_SET);
	write(velfd, "0", 2);
}

int makePipe(void)
{
	unlink("s_b_w_input");
	int errCode1 = mknod("s_b_w_input", 0010000, 0);
	chmod("s_b_w_input", 0660);
	unlink("t_c_input");
	int errCode2 = mknod("t_c_input", 0010000, 0);
	chmod("t_c_input", 0660);
	unlink("b_b_w_input");
	int errCode3 = mknod("b_b_w_input", 0010000, 0);
	chmod("b_b_w_input", 0660);
	return (errCode1 == -1 || errCode2 == -1 || errCode3 == -1);
}

void parkHandler(int sig)
{
	// cambio il process group id e eseguo l'aborto dei processi del gruppo che apparteneva ECUoutput, cioè i processi figli che gestiscono le richieste da f_w_c
	setpgid(0, getpgid(getppid()));
	kill(-getpid(), SIGABRT);
	while (!parking())
	{
		;
	}
	closeAll();
	kill(getppid(), SIGTERM);
	kill(ECUinputpid, SIGTERM);
	while (wait(NULL) > 0)
	{
		;
	}
	printf("ECUoutput ha concluso la sua esecuzione\n");
	write(debugLogfd, "ECUoutput ha concluso la sua esecuzione\n", 40);
	close(debugLogfd);
	exit(EXIT_SUCCESS);
}

int parking(void)
{
	char velocityStr[100];
	unsigned short inputFromp_a[100];
	char inputFromp_aS[100];
	int velocity;
	int success = 1;
	write(debugLogfd, "ECU: tentativo di parcheggio...\n", 32);
	// avvio la procedura di parking
	lseek(velfd, 0, SEEK_SET);
	readString(velfd, velocityStr);
	velocity = strtol(velocityStr, NULL, 10);
	// invio richieste di decelerazione a b_b_w finché la velocità non raggiunge 0
	while (velocity != 0)
	{
		write(ECUlogfd, "FRENO 5\n", 8);
		write(ECUoutputfd, "FRENO 5", 8);
		write(b_b_winputfd, "FRENO 5", 8);
		sleep(1);
		velocity -= 5;
		sprintf(velocityStr, "%d", velocity);
		lseek(velfd, 0, SEEK_SET);
		write(velfd, velocityStr, strlen(velocityStr) + 1);
	}
	// creo il pipe di comunicazione con park_assist
	unlink("p_a_output");
	int errCode = mknod("p_a_output", 0010000, 0);
	if (errCode)
	{
		perror("parking-mknod");
		exit(EXIT_FAILURE);
	}
	chmod("p_a_output", 0660);

	// prima di mettere in esecuzione park_assist vado rimuovere tutti i sensori e attuatori
	kill(t_cpid, SIGTERM);
	kill(s_b_wpid, SIGTERM);
	kill(b_b_wpid, SIGTERM);
	kill(f_w_cpid, SIGTERM);

	// metto in esecuzione park assist
	p_apid = fork();
	if (p_apid == 0)
	{
		execl("park_assist", "park_assist", &modality, NULL);
	}
	p_aoutputfd = open("p_a_output", O_RDONLY);

	// converto i numeri da esadecimale in decimale senza segno
	int a = (int)strtol("172A", NULL, 16);
	int b = (int)strtol("D693", NULL, 16);
	int c = (int)strtol("0000", NULL, 16);
	int d = (int)strtol("BDD8", NULL, 16);
	int e = (int)strtol("FAEE", NULL, 16);
	int f = (int)strtol("4300", NULL, 16);

	char temp1[100];

	// leggo l'output di park assist per stabilire il successo o no dell'operazione
	while (read(p_aoutputfd, inputFromp_a, 8) > 0)
	{
		// controllo i valori ottenuti
		sprintf(temp1, "ECU: valori trasmessi da p_a %d %d %d %d\n", inputFromp_a[0], inputFromp_a[1], inputFromp_a[2], inputFromp_a[3]);
		write(debugLogfd, temp1, strlen(temp1));
		int thereIsA = (inputFromp_a[0] == a || inputFromp_a[1] == a || inputFromp_a[2] == a || inputFromp_a[3] == a);
		int thereIsB = (inputFromp_a[0] == b || inputFromp_a[1] == b || inputFromp_a[2] == b || inputFromp_a[3] == b);
		int thereIsC = (inputFromp_a[0] == c || inputFromp_a[1] == c || inputFromp_a[2] == c || inputFromp_a[3] == c);
		int thereIsD = (inputFromp_a[0] == d || inputFromp_a[1] == d || inputFromp_a[2] == d || inputFromp_a[3] == d);
		int thereIsE = (inputFromp_a[0] == e || inputFromp_a[1] == e || inputFromp_a[2] == e || inputFromp_a[3] == e);
		int thereIsF = (inputFromp_a[0] == f || inputFromp_a[1] == f || inputFromp_a[2] == f || inputFromp_a[3] == f);
		read(p_aoutputfd, inputFromp_aS, 2);

		if (strcmp(inputFromp_aS, "T") == 0)
		{
			read(p_aoutputfd, inputFromp_a, 8);
			sprintf(temp1, "ECU: valori trasmessi da s_v_c %d %d %d %d\n", inputFromp_a[0], inputFromp_a[1], inputFromp_a[2], inputFromp_a[3]);
			write(debugLogfd, temp1, strlen(temp1));
			thereIsA = thereIsA || (inputFromp_a[0] == a || inputFromp_a[1] == a || inputFromp_a[2] == a || inputFromp_a[3] == a);
			thereIsB = thereIsB || (inputFromp_a[0] == b || inputFromp_a[1] == b || inputFromp_a[2] == b || inputFromp_a[3] == b);
			thereIsC = thereIsC || (inputFromp_a[0] == c || inputFromp_a[1] == c || inputFromp_a[2] == c || inputFromp_a[3] == c);
			thereIsD = thereIsD || (inputFromp_a[0] == d || inputFromp_a[1] == d || inputFromp_a[2] == d || inputFromp_a[3] == d);
			thereIsE = thereIsE || (inputFromp_a[0] == e || inputFromp_a[1] == e || inputFromp_a[2] == e || inputFromp_a[3] == e);
			thereIsF = thereIsF || (inputFromp_a[0] == f || inputFromp_a[1] == f || inputFromp_a[2] == f || inputFromp_a[3] == f);
		}
		else if (strcmp(inputFromp_aS, "F") == 0)
		{
			write(debugLogfd, "ECU: nessun valore trasmesso da s_v_c\n", 38);
		}

		if (thereIsA || thereIsB || thereIsC || thereIsD || thereIsE || thereIsF)
		{
			success = 0;
			break;
		}
	}

	close(p_aoutputfd);
	unlink("p_a_output");
	// termino l'esecuzione di park assist
	return success;
}

void throttleFailureHandler(int sig)
{
	write(ECUoutputfd, "FALLIMENTO", 11);
	lseek(velfd, 0, SEEK_SET);
	write(velfd, "0", 2);
	write(ECUoutputfd, "TERMINAZIONE", 13);
	kill(0, SIGTERM);
	kill(-getpgid(getppid()), SIGTERM);
}