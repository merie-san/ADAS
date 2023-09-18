#include "headers.h"

void exitHandler(int);

int camerasLogfd, debugLogfd, urandomNfd, urandomAfd, s_v_coutputfd;

int main(int argc, char const *argv[])
{
	unsigned short inputFromFile[100];
	char inputToLog1[100], inputToLog2[100], inputToLog3[100], inputToLog4[100];
	// apro i file di log
	debugLogfd = open("../log/debuglog.txt", O_CREAT | O_APPEND | O_WRONLY, 0660);
	printf("surround_view_camera in esecuzione...\n");
	write(debugLogfd, "surround_view_camera in esecuzione...\n", 38);
	camerasLogfd = open("../log/cameras.log", O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0660);
	// controllo gli errori e apro i file
	if (camerasLogfd == -1)
	{
		perror("s_v_c-camerasLog");
		exit(EXIT_FAILURE);
	}

	urandomNfd = open("/dev/urandom", O_RDONLY);
	if (urandomNfd == -1)
	{
		perror("p_a-urandomN");
		exit(EXIT_FAILURE);
	}

	urandomAfd = open("../data/urandomARTIFICIALE.binary", O_RDONLY);
	if (urandomAfd == -1)
	{
		perror("p_a-urandomA");
		exit(EXIT_FAILURE);
	}

	// apro il pipe di comunicazione con park_assist
	s_v_coutputfd = open("s_v_coutput", O_WRONLY);
	signal(SIGTERM, exitHandler);

	// trasmetto i dati a park_assist
	int bytesRead;
	while (1)
	{
		if (*argv[1] == 0)
		{
			bytesRead = read(urandomNfd, inputFromFile, 8);
		}
		else
		{
			bytesRead = read(urandomAfd, inputFromFile, 8);
		}

		if (bytesRead == 8)
		{
			write(s_v_coutputfd, "T", 2);
			write(s_v_coutputfd, inputFromFile, 8);
			sprintf(inputToLog1, "%u\n", inputFromFile[0]);
			sprintf(inputToLog2, "%u\n", inputFromFile[1]);
			sprintf(inputToLog3, "%u\n", inputFromFile[2]);
			sprintf(inputToLog4, "%u\n", inputFromFile[3]);
			write(camerasLogfd, inputToLog1, strlen(inputToLog1));
			write(camerasLogfd, inputToLog2, strlen(inputToLog2));
			write(camerasLogfd, inputToLog3, strlen(inputToLog3));
			write(camerasLogfd, inputToLog4, strlen(inputToLog4));
			sleep(1);
		}
		else
		{
			write(s_v_coutputfd, "F", 2);
			write(debugLogfd, "s_v_c: numero byte letti insufficienti\n", 39);
		}
	}
	return 0;
}

void exitHandler(int sig)
{
	close(s_v_coutputfd);
	close(camerasLogfd);
	close(urandomAfd);
	close(urandomNfd);
	printf("surround_view_cameras ha concluso la sua esecuzione\n");
	write(debugLogfd, "surround_view_cameras ha concluso la sua esecuzione\n", 52);
	close(debugLogfd);
	exit(EXIT_SUCCESS);
}
