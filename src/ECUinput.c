#include "headers.h"

void quitHandler(int);
int p_apid, debugLogfd;

int main(int argc, char const *argv[])
{
	debugLogfd = open("../log/debuglog.txt", O_CREAT | O_APPEND | O_WRONLY, 0660);
	write(debugLogfd, "avvio ECUinput...\n", 18);
	printf("avvio ECUinput...\n");

	// installo il signal handler per la gestione della terminazione del programma
	signal(SIGTERM, quitHandler);

	while (1)
	{
		char *inputFromHMI = (char *)argv[2];
		readString(*argv[1], inputFromHMI);
		if (strcmp(argv[2], "PARCHEGGIO") == 0)
		{
			kill(getppid(), SIGUSR2);
			break;
		}
		if (strcmp(inputFromHMI, "ARRESTO") == 0)
		{
			kill(getppid(), SIGUSR1);
		}
	}
	exit(EXIT_SUCCESS);
}

void quitHandler(int sig)
{
	printf("ECUinput ha concluso la sua esecuzione\n");
	write(debugLogfd, "ECUinput ha concluso la sua esecuzione\n", 39);
	close(debugLogfd);
	exit(EXIT_SUCCESS);
}
