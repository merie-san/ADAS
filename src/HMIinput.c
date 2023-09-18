#include "headers.h"

int startECU(int);
int makePipe(void);
void quitHandler(int);
void ECUkiller(int);
int ECUinputfd, ECUoutputpid, debugLogfd;

int main(int argc, char const *argv[])
{
	int modalita = 0, HMIoutputpid;
	char inputDaTerminale[100];
	debugLogfd = open("../log/debuglog.txt", O_CREAT | O_APPEND | O_WRONLY, 0660);

	printf("programma in esecuzione...\n");
	write(debugLogfd, "\nprogramma in esecuzione...\n", 28);
	printf("HMI in esecuzione...\n");
	write(debugLogfd, "HMI in esecuzione...\n", 21);
	printf("HMIinput in esecuzione...\n");
	write(debugLogfd, "HMIinput in esecuzione...\n", 26);
	sleep(1);

	if (argv[1] == NULL)
	{
		printf("argomento illegale, esecuzione impostata in modalità normale\n");
	}
	else if (strcmp(argv[1], "ARTIFICIALE") == 0)
	{
		modalita = 1;
	}
	else
	{
		if (strcmp(argv[1], "NORMALE") != 0)
		{
			printf("argomento illegale, esecuzione impostata in modalità normale\n");
		};
	}

	if (makePipe())
	{
		perror("makePipe");
		exit(EXIT_FAILURE);
	}

	// installo il nuovo handler per la gestione della terminazione del programma
	signal(SIGTERM, quitHandler);
	signal(SIGINT, ECUkiller);

	// avvio l'esecuzione dell'ECU, passandogli la modalità di esecuzione
	ECUoutputpid = startECU(modalita);

	if (ECUoutputpid == -1)
	{
		perror("ECUoutputpid");
		exit(EXIT_FAILURE);
	}

	// apertura terminale per l'esecuzione di HMIoutput
	system("gnome-terminal -- ./HMIoutput");

	ECUinputfd = open("ECU_input", O_WRONLY);
	sleep(2);

	while (1)
	{
		printf("HMIinput: inserire \"INIZIO\" per iniziare la navigazione\n");
		scanf("%s", inputDaTerminale);
		if (strcmp(inputDaTerminale, "INIZIO") == 0)
		{
			write(ECUinputfd, inputDaTerminale, strlen(inputDaTerminale) + 1);
			break;
		}
	}

	while (1)
	{
		scanf("%s", inputDaTerminale);
		int isParcheggio = (strcmp(inputDaTerminale, "PARCHEGGIO") == 0);
		int isArresto = (strcmp(inputDaTerminale, "ARRESTO") == 0);

		if (isParcheggio || isArresto)
		{
			write(ECUinputfd, inputDaTerminale, strlen(inputDaTerminale) + 1);
		}
		else
		{
			printf("HMIinput: comando illegale\n");
		}
	}

	return 0;
}

int startECU(int modalita)
{
	int pid = fork();
	int *mod = &modalita;
	if (pid == 0)
	{
		execl("ECUoutput", "ECUoutput", mod, NULL);
		return 1;
	}
	return pid;
}

int makePipe(void)
{
	int errCode1, errCode2;
	unlink("ECU_output");
	unlink("ECU_input");
	errCode1 = mknod("ECU_output", 0010000, 0);
	chmod("ECU_output", 0660);
	errCode2 = mknod("ECU_input", 0010000, 0);
	chmod("ECU_input", 0660);
	return (errCode1 == -1 || errCode2 == -1);
}

void quitHandler(int sig)
{
	close(ECUinputfd);
	while (wait(NULL) > 0)
	{
		;
	}
	printf("HMIinput ha concluso la sua esecuzione\n");
	write(debugLogfd, "HMIinput ha concluso la sua esecuzione\n", 39);
	printf("il programma ha concluso la sua esecuzione\n");
	write(debugLogfd, "il programma ha concluso la sua esecuzione\n\n", 43);
	close(debugLogfd);
	exit(EXIT_SUCCESS);
}

void ECUkiller(int sig)
{
	kill(ECUoutputpid, SIGINT);
	exit(EXIT_SUCCESS);
}