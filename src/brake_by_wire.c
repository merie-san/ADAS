#include "headers.h"

void quitHandler(int);
void stopHandler(int);

int brakeLogfd, b_b_winputfd, debugLogfd;

int main(int argc, char const *argv[])
{
	char inputFromECU[100], outputToLog[100];

	debugLogfd = open("../log/debuglog.txt", O_CREAT | O_WRONLY | O_APPEND, 0660);
	printf("brake_by_wire in esecuzione...\n");
	write(debugLogfd, "brake_by_wire in esecuzione...\n", 31);

	// apro il file di log
	unlink("brake.log");
	brakeLogfd = open("../log/brake.log", O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0660);
	if (brakeLogfd == -1)
	{
		perror("b_b_w-openLogFile");
		exit(EXIT_FAILURE);
	}

	// apro il pipe di comunicazione con l'ECU
	b_b_winputfd = open("b_b_w_input", O_RDONLY);
	if (b_b_winputfd == -1)
	{
		perror("b_b_w-openpipe");
		exit(EXIT_FAILURE);
	}
	write(debugLogfd, "b_b_w: aperta comunicazione con ECU\n", 36);

	// installo gli handler
	signal(SIGTERM, quitHandler);
	signal(SIGUSR1, stopHandler);

	// gestione richieste dalla ECU
	while (readString(b_b_winputfd, inputFromECU))
	{
		if (strcmp(inputFromECU, "FRENO 5") == 0)
		{
			char *date = __DATE__;
			sprintf(outputToLog, "%s: FRENO 5\n", date);
			write(brakeLogfd, outputToLog, strlen(outputToLog));
		}
	}

	// fine programma
	close(brakeLogfd);
	close(b_b_winputfd);
	printf("conclusione esecuzione ECU, chiusura brake_by_wire...\n");
	write(debugLogfd, "conclusione esecuzione ECU, chiusura brake_by_wire...\n", 54);
	close(debugLogfd);
	exit(EXIT_SUCCESS);
	return 0;
}

void quitHandler(int sig)
{
	close(brakeLogfd);
	close(b_b_winputfd);
	write(debugLogfd, "brake_by_wire ha concluso la sua esecuzione\n", 44);
	printf("brake_by_wire ha concluso la sua esecuzione\n");
	close(debugLogfd);
	exit(EXIT_SUCCESS);
}

void stopHandler(int sig)
{
	write(brakeLogfd, "ARRESTO AUTO\n", 13);
}