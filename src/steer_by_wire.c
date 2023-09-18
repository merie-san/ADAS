#include "headers.h"

void quitHandler(int);
int readUnblock(int, char *);

int steerLogfd, s_b_winputfd, ECUsemfd, debugLogfd;

int main(int argc, char const *argv[])
{
	char inputFromECU[100];
	debugLogfd = open("../log/debuglog.txt", O_CREAT | O_APPEND | O_WRONLY, 0660);
	printf("steer_by_wire in esecuzione...\n");
	write(debugLogfd, "steer_by_wire in esecuzione...\n", 31);
	// apri il log e il pipe di comunicazione con l'ECU
	steerLogfd = open("../log/steer.log", O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0660);
	if (steerLogfd == -1)
	{
		perror("s_b_w-openLogFile");
		exit(EXIT_FAILURE);
	}

	// installo un nuovo handler per la terminazione del processo
	signal(SIGTERM, quitHandler);

	// semaforo ECU per la sincronizzazione processi (aspetto finché il semaforo venga creato)
	do
	{
		ECUsemfd = open("ECU_SBW_semaphore", O_WRONLY);
		if (ECUsemfd == -1)
		{
			sleep(1);
		}
	} while (ECUsemfd == -1);

	// apro il pipe di comunicazione tra ECU e s_b_w
	s_b_winputfd = open("s_b_w_input", O_RDONLY);
	fcntl(s_b_winputfd, F_SETFL, O_NONBLOCK);
	if (s_b_winputfd == -1)
	{
		perror("s_b_w-openpipe");
		exit(EXIT_FAILURE);
	}
	write(debugLogfd, "s_b_w: aperta comunicazione con ECU\n", 36);

	while (1)
	{
		// provo a leggere dal pipe O_NONBLOCK: se non riesco a leggere niente allora stampo "NO ACTION" nel file log
		int res = readUnblock(s_b_winputfd, inputFromECU);
		if (res == -1)
		{
			write(steerLogfd, "NO ACTION\n", 10);
			sleep(1);
		}
		else if (res == 1)
		{
			if (strcmp(inputFromECU, "DESTRA") == 0)
			{
				write(steerLogfd, "STO GIRANDO A DESTRA\n", 21);
				sleep(1);
				write(ECUsemfd, "OK", 2);
			}
			else if (strcmp(inputFromECU, "SINISTRA") == 0)
			{
				write(steerLogfd, "STO GIRANDO A SINISTRA\n", 23);
				sleep(1);
				write(ECUsemfd, "OK", 2);
			}
		}
		else if (res == 0)
		{
			break;
		}
	}

	close(steerLogfd);
	close(ECUsemfd);
	close(s_b_winputfd);
	printf("conclusione esecuzione ECU, chiusura steer_by_wire...\n");
	write(debugLogfd, "conclusione esecuzione ECU, chiusura steer_by_wire...\n", 54);
	close(debugLogfd);
	return 0;
}

int readUnblock(int fd, char *str)
{
	int res, i;
	res = read(fd, str, 1);
	while (res > 0 && *(str + i) != '\0')
	{
		i++;
		res = read(fd, str + i, 1);
	}
	return res;
}

void quitHandler(int sig)
{
	close(ECUsemfd);
	close(steerLogfd);
	close(s_b_winputfd);
	printf("steer_by_wire ha concluso la sua esecuzione\n");
	write(debugLogfd, "steer_by_wire ha concluso la sua esecuzione\n", 44);
	close(debugLogfd);
	exit(EXIT_SUCCESS);
}
