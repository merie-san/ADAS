#include "headers.h"

int assistLogfd, urandomNfd, urandomAfd, p_aoutputfd, debugLogfd, s_v_coutputfd;
int s_v_cpid;

int main(int argc, char const *argv[])
{
	unsigned short inputFromFile[100], inputFroms_v_c[100];
	char inputToLog1[100], inputToLog2[100], inputToLog3[100], inputToLog4[100], inputFroms_v_cF[100];

	// apri i file
	debugLogfd = open("../log/debuglog.txt", O_CREAT | O_APPEND | O_WRONLY, 0660);
	printf("park_assist in esecuzione...\n");
	write(debugLogfd, "park_assist in esecuzione...\n", 29);
	assistLogfd = open("../log/assist.log", O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, 0660);
	if (assistLogfd == -1)
	{
		perror("p_a-assistLog");
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

	// apro il pipe di comunicazione con l'ECU e creo quello con surround_view_camera
	p_aoutputfd = open("p_a_output", O_WRONLY);
	write(debugLogfd, "p_a: aperta comunicazione con ECU\n", 34);
	unlink("s_v_coutput");
	mknod("s_v_coutput", 0010000, 0);
	chmod("s_v_coutput", 0660);

	// avvia surround_view_cameras
	s_v_cpid = fork();
	if (s_v_cpid == 0)
	{
		execl("surround_view_cameras", "surround_view_cameras", argv[1], NULL);
		exit(EXIT_FAILURE);
	}

	// apro pipe di comunicazione con surround_view_cameras
	s_v_coutputfd = open("s_v_coutput", O_RDONLY);
	write(debugLogfd, "p_a: aperta comunicazione con s_v_c\n", 36);

	for (int i = 0; i < 30; i++)
	{
		if (*argv[1] == 0)
		{
			read(urandomNfd, inputFromFile, 8);
		}
		else
		{
			read(urandomAfd, inputFromFile, 8);
		}

		// scrivo sull'output allp'ECU
		write(p_aoutputfd, inputFromFile, 8);

		// scrivo i dati raccolti sul log
		sprintf(inputToLog1, "p_a: %u\n", inputFromFile[0]);
		sprintf(inputToLog2, "p_a: %u\n", inputFromFile[1]);
		sprintf(inputToLog3, "p_a: %u\n", inputFromFile[2]);
		sprintf(inputToLog4, "p_a: %u\n", inputFromFile[3]);
		write(assistLogfd, inputToLog1, strlen(inputToLog1));
		write(assistLogfd, inputToLog2, strlen(inputToLog2));
		write(assistLogfd, inputToLog3, strlen(inputToLog3));
		write(assistLogfd, inputToLog4, strlen(inputToLog4));
		read(s_v_coutputfd, inputFroms_v_cF, 2);

		if (strcmp(inputFroms_v_cF, "T") == 0)
		{
			read(s_v_coutputfd, inputFroms_v_c, 8);
			sprintf(inputToLog1, "s_v_c: %u\n", inputFroms_v_c[0]);
			sprintf(inputToLog2, "s_v_c: %u\n", inputFroms_v_c[1]);
			sprintf(inputToLog3, "s_v_c: %u\n", inputFroms_v_c[2]);
			sprintf(inputToLog4, "s_v_c: %u\n", inputFroms_v_c[3]);
			write(assistLogfd, inputToLog1, strlen(inputToLog1));
			write(assistLogfd, inputToLog2, strlen(inputToLog2));
			write(assistLogfd, inputToLog3, strlen(inputToLog3));
			write(assistLogfd, inputToLog4, strlen(inputToLog4));
			write(p_aoutputfd, "T", 2);
			write(p_aoutputfd, inputFroms_v_c, 8);
		}
		else if (strcmp(inputFroms_v_cF, "F") == 0)
		{
			write(debugLogfd, "p_a: nessun dato inviato da s_v_c\n", 34);
			write(p_aoutputfd, "F", 2);
		}
		sleep(1);
	}

	// chiudo i file e termino surround_view_cameras prima di concludere
	close(p_aoutputfd);
	close(assistLogfd);
	close(urandomAfd);
	close(urandomNfd);
	close(s_v_coutputfd);
	unlink("s_v_coutput");
	kill(s_v_cpid, SIGTERM);
	wait(NULL);
	printf("park_assist ha concluso la sua esecuzione\n");
	write(debugLogfd, "park_assist ha concluso la sua esecuzione\n", 42);
	close(debugLogfd);
	return 0;
}
