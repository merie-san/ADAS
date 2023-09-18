#include "headers.h"

void quitHandler(int);
long generate(void);
int throttleLogfd, t_cinputfd, debugLogfd;

int main(int argc, char const *argv[])
{
	char inputFromECU[100], outputToLog[100];
	debugLogfd = open("../log/debuglog.txt", O_CREAT | O_APPEND | O_WRONLY, 0660);
	printf("throttle_control in esecuzione...\n");
	write(debugLogfd, "throttle_control in esecuzione...\n", 34);

	// apro il file di log
	unlink("throttle.log");
	throttleLogfd = open("../log/throttle.log", O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0660);
	if (throttleLogfd == -1)
	{
		perror("t_c-openLogFile");
		exit(EXIT_FAILURE);
	}

	// apro il pipe di comunicazione con l'ECU
	t_cinputfd = open("t_c_input", O_RDONLY);
	if (t_cinputfd == -1)
	{
		perror("t_c-openpipe");
		exit(EXIT_FAILURE);
	}
	write(debugLogfd, "t_c: aperta comunicazione con ECU\n", 34);

	// installo l'handler per la terminazione del programma
	signal(SIGTERM, quitHandler);

	// gestione delle richieste dall'ECU
	while (readString(t_cinputfd, inputFromECU))
	{
		if (strcmp(inputFromECU, "INCREMENTO 5") == 0)
		{
			if (generate() % 100000 != 0)
			{
				char *date = __DATE__;
				sprintf(outputToLog, "%s: AUMENTO 5\n", date);
				write(throttleLogfd, outputToLog, strlen(outputToLog));
			}
			else
			{
				char *date = __DATE__;
				sprintf(outputToLog, "%s: FALLIMENTO ACCELERATORE\n", date);
				write(throttleLogfd, outputToLog, strlen(outputToLog));
				kill(getppid(), SIGQUIT);
			}
		}
	}

	// fine programma
	close(throttleLogfd);
	close(t_cinputfd);
	printf("conclusione esecuzione ECU, chiusura throttle_control...\n");
	write(debugLogfd, "conclusione esecuzione ECU, chiusura throttle_control...\n", 57);
	close(debugLogfd);
	exit(EXIT_SUCCESS);
	return 0;
}

void quitHandler(int sig)
{
	close(throttleLogfd);
	close(t_cinputfd);
	printf("throttle_control ha concluso la sua esecuzione\n");
	write(debugLogfd, "throttle_control ha concluso la sua esecuzione\n", 47);
	close(debugLogfd);
	exit(EXIT_SUCCESS);
}

long generate(void)
{
	time_t t;
	srand((unsigned)time(&t));
	return rand();
}
