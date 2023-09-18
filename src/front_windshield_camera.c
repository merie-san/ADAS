#include "headers.h"

int readLine(int, char *);
void quitHandler(int);

int clientfd, frontCamerafd, debugLogfd;

int main(int argc, char const *argv[])
{
	int serverLen, errCode;
	char inputFromFile[100], wait[10];
	struct sockaddr_un serverAddress;
	struct sockaddr *serverAddrPtr;

	debugLogfd = open("../log/debuglog.txt", O_CREAT | O_APPEND | O_WRONLY, 0660);
	write(debugLogfd, "front_windshield_camera in esecuzione...\n", 41);
	printf("front_windshield_camera in esecuzione...\n");

	// installo il quithandler
	signal(SIGTERM, quitHandler);

	// creazione socket lato client
	serverAddrPtr = (struct sockaddr *)&serverAddress;
	serverLen = sizeof(serverAddress);
	clientfd = socket(AF_UNIX, SOCK_STREAM, 0);
	serverAddress.sun_family = AF_UNIX;
	strcpy(serverAddress.sun_path, "ECUServer");
	write(debugLogfd, "client f_w_c creato\n", 20);

	// lettura file
	frontCamerafd = open("../data/frontCamera.data", O_RDONLY);
	if (frontCamerafd == -1)
	{
		perror("frontCamerafd");
		exit(EXIT_FAILURE);
	}

	int charsRead = readLine(frontCamerafd, inputFromFile);
	char *temp1;
	char temp2[100];
	while (charsRead > 0)
	{
		temp1 = inputFromFile;
		sprintf(temp2, "f_w_c: avviato tentativo di connessione all'ECU...\nmessaggio inviato all'ECU: %s\n", temp1);
		write(debugLogfd, temp2, strlen(temp2));
		do
		{
			errCode = connect(clientfd, serverAddrPtr, serverLen);
			if (errCode == -1)
			{
				sleep(1);
			}
		} while (errCode == -1);

		write(debugLogfd, "f_w_c: collegamento con l'ECU avvenuto con successo\n", 52);
		write(clientfd, inputFromFile, strlen(inputFromFile) + 1);

		// imposto la if per bloccare operazioni che non possono svolgersi contemporaneamente
		int wasPARCHEGGIO = (strcmp(inputFromFile, "PARCHEGGIO") == 0);
		int wasDIREZIONE = (strcmp(inputFromFile, "DESTRA") == 0 || strcmp(inputFromFile, "SINISTRA") == 0);
		int wasVELOCITA = (!wasPARCHEGGIO && !wasDIREZIONE);
		charsRead = readLine(frontCamerafd, inputFromFile);
		int isDIREZIONE = (strcmp(inputFromFile, "DESTRA") == 0 || strcmp(inputFromFile, "SINISTRA") == 0);

		if (!(charsRead > 0 && wasVELOCITA && isDIREZIONE))
		{
			// aspetto che la richiesta venga gestita prima di procedere
			write(debugLogfd, "f_w_c: in attesa del completamento dell'operazione richiesta...\n", 64);
			read(clientfd, wait, 1);
			close(clientfd);
		}

		clientfd = socket(AF_UNIX, SOCK_STREAM, 0);
		sleep(1);
	}
	printf("front_windshield_camera ha concluso la sua esecuzione\n");
	write(debugLogfd, "front_windshield_camera ha concluso la sua esecuzione\n", 54);
	close(debugLogfd);
	close(frontCamerafd);
	exit(EXIT_SUCCESS);
	return 0;
}

int readLine(int fd, char *str)
{
	int n = 0, d = 0, i = 0;
	d = read(fd, str, 1);
	for (; (*(str + i) != '\0') && (*(str + i) != '\n') && (d > 0); i++, n += d)
	{
		d = read(fd, str + i + 1, 1);
	}
	*(str + i) = '\0';
	return n;
}

void quitHandler(int sig)
{
	close(clientfd);
	close(frontCamerafd);
	write(debugLogfd, "front_windshield_camera ha concluso la sua esecuzione\n", 54);
	close(debugLogfd);
	printf("front_windshield_camera ha concluso la sua esecuzione\n");
	exit(EXIT_SUCCESS);
}
