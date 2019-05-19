#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <fcntl.h>
#define MAX 300 
#define PORT 8080 
#define SA struct sockaddr 

char buffer[MAX];
int socketDescriptor; 
struct sockaddr_in servaddr;
struct stat fileStatus; 
int filehandle;
char command[5], filename[20];
int i = 1;
char *f;
int fileSize, status;

int userLogin(int socketDescriptor);
void listDirectoryContent();
void changeDirectory();
int downloadFromServer();
int uploadToServer();
void createAndConnectSocket();

void createAndConnectSocket()
{
	socketDescriptor = socket(AF_INET, SOCK_STREAM, 0); 
	if (socketDescriptor == -1) 
	{ 
		printf("Failed to create socket!\n"); 
		exit(0); 
	} 
	else
	{
		printf("Socket successfully created!\n"); 
	}
	bzero(&servaddr, sizeof(servaddr)); 

	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	servaddr.sin_port = htons(PORT); 

	if (connect(socketDescriptor, (SA*)&servaddr, sizeof(servaddr)) != 0) 
	{ 
		printf("Failed to connect to the server!\n"); 
		exit(0); 
	} 
	else
	{
		printf("Successfully connected to the server!\n"); 
	}
}

int uploadToServer()
{
	printf("Enter the name of the file you want to upload to the server: ");
        scanf("%s", filename);
	filehandle = open(filename, O_RDONLY);
        if(filehandle == -1)
        {
              	printf("No such file on the local directory!\n");
              	return 0;
        }
        strcpy(buffer, "put ");
	strcat(buffer, filename);
	send(socketDescriptor, buffer, 100, 0);
	stat(filename, &fileStatus);
	fileSize = fileStatus.st_size;
	send(socketDescriptor, &fileSize, sizeof(int), 0);
	sendfile(socketDescriptor, filehandle, NULL, fileSize);
	recv(socketDescriptor, &status, sizeof(int), 0);
	if(status) 
	{
		printf("File successfully uploaded to the server!\n");
	}
	else
	{
	    	printf("File failed to be uploaded to the server!\n");
	}
}

int downloadFromServer()
{
	printf("Enter the name of the file you want to download from the server: ");
	scanf("%s", filename);
	strcpy(buffer, "get ");
	strcat(buffer, filename);
	send(socketDescriptor, buffer, 100, 0);
	recv(socketDescriptor, &fileSize, sizeof(int), 0);
	if(!fileSize)
	{
		printf("No such file on the remote server!\n");
		return 0;
	}
	f = malloc(fileSize);
	recv(socketDescriptor, f, fileSize, 0);
	while(1)
	{
	      	filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
	      	if(filehandle == -1)
		{
		  	sprintf(filename + strlen(filename), "%d", i);
		}
	      	else break;
	}
	write(filehandle, f, fileSize, 0);
	printf("File successfully downloaded!\n");
	close(filehandle);
}

void changeDirectory()
{
	char directoryName[200];
	strcpy(buffer, "cd ");
	printf("Enter directory name: ");
	scanf("%s", directoryName);
	strcat(buffer, directoryName);
	send(socketDescriptor, buffer, 100, 0);
	recv(socketDescriptor, &status, sizeof(int), 0);
	if (status) 
	{
	        printf("Remote directory successfully changed!\n");
	}
	else 
	{
		printf("Failed to change the remote directory!\n");
	}
}

void listDirectoryContent()
{
	strcpy(buffer, "ls");
        send(socketDescriptor, buffer, 100, 0);
	recv(socketDescriptor, &fileSize, sizeof(int), 0);
        f = malloc(fileSize);
        recv(socketDescriptor, f, fileSize, 0);
	filehandle = creat("temp.txt", O_WRONLY);
	write(filehandle, f, fileSize);
	close(filehandle);
        printf("Content of remote directory:\n");
	system("cat temp.txt");
}

int userLogin(int socketDescriptor) 
{
	char username[100];
	char password[100];
	char buffer[100];
	char usernameAndPassword[300];
	int hasUser = 0;
	for(int i = 0; i < 3; i++)
	{
		printf("Login credentials:\n");
		printf("Enter username: ");
		scanf("%s", username);
		printf("Enter password: ");
		scanf("%s", password);
		
		bzero(usernameAndPassword, 300);

		strcat(usernameAndPassword, username);
		strcat(usernameAndPassword, " ");
		strcat(usernameAndPassword, password);
		
		write(socketDescriptor, usernameAndPassword, sizeof(usernameAndPassword));

		bzero(buffer, 100);
		read(socketDescriptor, buffer, sizeof(buffer));
		
		if(strcmp(buffer, "Yes") == 0)
		{
			hasUser = 1;
			break;
		}
		else
		{
			printf("Wrong username or password! Try again!\n");
			printf("Attempts left: %d\n", 2 - i);
		}
	 }
	return hasUser;
}

int main() 
{ 	
	createAndConnectSocket();

 	int choice = 0, hasError = 0;

	if (userLogin(socketDescriptor) == 1) 
	{
		while(1)
		{
			printf("----------Menu----------\n");
	   		printf("1. List content of the current directory\n");
			printf("2. Change directory\n");
			printf("3. Download file from remote server\n");
			printf("4. Upload file to remote server\n");
			printf("5. Exit\n");
			printf("------------------------\n");
			
			printf("Choose an option: ");
	   		scanf("%d",&choice);
	   		switch(choice)
			{
	    		case 1:
 	   			listDirectoryContent();
				break;
	   		case 2:
				changeDirectory();
				break;
	    		case 3:
				downloadFromServer();
	  			break;
			case 4:
				hasError = uploadToServer();
	  			break;
			case 5:
				strcpy(buffer, "exit");
				send(socketDescriptor, buffer, 100, 0);
				close(socketDescriptor);
				printf("Client exited!\n");
				exit(0);
			}
     		}
   	}
	else 
	{
		close(socketDescriptor);
		printf("Reached maxuimum login attempts!\n");
		printf("Server closed the connection!\n");
		exit(1);
	}
}
