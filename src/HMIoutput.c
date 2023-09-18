#include "headers.h"

int debugLogfd;

int main(int argc, char const *argv[])
{
	int ECUoutputfd;
	char HMIbuf[100], outputBuf[100];
	debugLogfd = open("../log/debuglog.txt", O_CREAT | O_WRONLY | O_APPEND, 0660);

	printf("HMIoutput in esecuzione...\n");
	write(debugLogfd, "HMIoutput in esecuzione...\n", 27);
	ECUoutputfd = open("ECU_output", O_RDONLY);
	
	while (readString(ECUoutputfd, HMIbuf))
	{
		printf("%s\n", HMIbuf);
	}

	// il pipe di comunicazione con l'ECU è stato chiuso, termino l'esecuzione
	close(ECUoutputfd);
	printf("conclusione esecuzione ECU, chiusura HMIoutput...\n");
	write(debugLogfd, "conclusione esecuzione ECU, chiusura HMIoutput...\n", 49);
	close(debugLogfd);
	exit(EXIT_SUCCESS);
	return 0;
}
