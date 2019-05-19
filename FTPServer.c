#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h> 
#include <fcntl.h>
#include <mysql/mysql.h>

#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr 

int socketDescriptor, conn, length; 
struct sockaddr_in servaddr, client;
int filehandle; 
char buffer[100], command[5], filename[20], destination[200];
struct stat fileStatus;
int fileSize, status, i = 1;

void createServerSocket();
int authenticateUser(char usernameAndPassword[]);
int connectToDB(char usernameAndPassword[]);
int getCredentials();
void listDirectory();
void changeDirectory();
void giveFileToClient();
void getFileFromClient();

void getFileFromClient()
{
	int status = 0;
	char *serverFile;

	sscanf(buffer, "%s%s", filename, filename);
	recv(conn, &fileSize, sizeof(int), 0);

	//i = 1;

	while(1)
	{
		filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
		if(filehandle == -1)
		{
			  sprintf(filename + strlen(filename), "%d", i);
		}
		else break;
	}

	serverFile = malloc(fileSize);
	recv(conn, serverFile, fileSize, 0);
	status = write(filehandle, serverFile, fileSize);
	close(filehandle);
	send(conn, &status, sizeof(int), 0);
}

void giveFileToClient()
{
	sscanf(buffer, "%s%s", filename, filename);
	stat(filename, &fileStatus);
	filehandle = open(filename, O_RDONLY);
	fileSize = fileStatus.st_size;

	if(filehandle == -1)
	{
	      	fileSize = 0;
	}
	send(conn, &fileSize, sizeof(int), 0);

	if(fileSize) 
	{
	  	sendfile(conn, filehandle, NULL, fileSize);
	}
}

void changeDirectory()
{
	bzero(destination, 200);
	sscanf(buffer, "%s %s", destination, destination);
			
	if(chdir(destination) == 0)
	{
		status = 1;
	}
	else 
	{
		status = 0;
	}

	send(conn, &status, sizeof(int), 0);
}

void listDirectory()
{
	system("ls >temps.txt");
	//i = 0;
	stat("temps.txt", &fileStatus);
	fileSize = fileStatus.st_size;
	send(conn, &fileSize, sizeof(int), 0);
	filehandle = open("temps.txt", O_RDONLY);
	sendfile(conn, filehandle, NULL, fileSize);
}

int getCredentials()
{
	int hasUser = 0;
	char usernameAndPassword[300];
	bzero(usernameAndPassword, 300);
	recv(conn, &usernameAndPassword, sizeof(usernameAndPassword), 0);
	hasUser = authenticateUser(usernameAndPassword);
	bzero(buffer, 100);
	if (hasUser == 1) 
	{
		strcpy(buffer, "Yes");
		send(conn, &buffer, sizeof(buffer), 0);
		return hasUser;
	}
	else 
	{
	   	strcpy(buffer, "No");
	   	send(conn, &buffer, sizeof(buffer), 0);
	} 
}

void createServerSocket()
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
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT); 
 
	if ((bind(socketDescriptor, (SA*)&servaddr, sizeof(servaddr))) != 0)
	{ 
		printf("Failed to bind socket!\n"); 
		exit(0); 
	} 
	else
	{
		printf("Socket successfully binded!\n"); 
	}
 
	if ((listen(socketDescriptor, 5)) != 0) 
	{ 
		printf("Listening failed!\n"); 
		exit(0); 
	} 
	else
	{
		printf("Server listening!\n"); 
	}

	length = sizeof(client); 

	conn = accept(socketDescriptor, (SA*)&client, &length); 
	if (conn < 0) 
	{ 
		printf("Server acccept failed!\n"); 
		exit(0); 
	} 
	else
	{
		printf("Server acccepted the client!\n"); 
	}
}

int authenticateUser(char usernameAndPassword[])
{	
	int hasUser = 0;

	hasUser = connectToDB(usernameAndPassword);

	return hasUser;	
}

int connectToDB(char usernameAndPassword[]) {
	char username[100];
	char password[100];
	char query[300];

	bzero(username, 100);
	bzero(password, 100);
	bzero(query, 300);

	sscanf(usernameAndPassword, "%s %s", username, password);

	MYSQL *con = mysql_init(NULL);
  	if (con == NULL) 
  	{
      		printf("Failed to establish connection to the database!");
      		exit(1);
  	}

  	if (!(mysql_real_connect(con, "localhost", "root", "19283758264", 
          	"SPRDataBase", 3306, NULL, 0))) 
  	{
      		printf("Failed to establish connection with given credentials!");
      		mysql_close(con);
      		exit(1);
  	}  	


  	if (mysql_query(con, "Select username, password from users")) 
  	{
      		printf("Wrong query!");
      		mysql_close(con);
      		exit(1);
  	}

	MYSQL_RES *result = mysql_store_result(con);
  
  	if (result == NULL) 
  	{
      		//finish_with_error(con);
  	}

  	int num_fields = mysql_num_fields(result);

  	MYSQL_ROW row;

	int hasUser = 0;

  	while ((row = mysql_fetch_row(result))) 
  	{ 
      		 
          	if(strcmp(row[0], username) == 0 && strcmp(row[1], password) == 0)
		{
			hasUser = 1;
			break;
		}	 
  	}

  	mysql_close(con);

	return hasUser;
}

int main() 
{ 
	createServerSocket();	

	int hasUser = 0;
	int attempts = 3;

	while(1) 
	{
		hasUser = getCredentials();
		if(hasUser == 1)
		{
			break;
		}
		else
		{
			attempts--;
		}
		if(attempts == 0)
		{
			printf("Server closed!");
			close(socketDescriptor);
			exit(1);
		}
	}

	//i = 1;

	while(1)
	{
      		recv(conn, buffer, 100, 0);
      		sscanf(buffer, "%s", command);

      		if(!strcmp(command, "ls"))
		{
			listDirectory();
		}

		else if (!strcmp(command, "cd")) 
		{
			changeDirectory();
		}

		else if(!strcmp(command, "get")) 
		{
			giveFileToClient();
		}

		else if(!strcmp(command, "put"))
		{
			getFileFromClient();
		}
		else if(!strcmp(command, "exit"))
		{
			printf("Server closed!\n");
			close(socketDescriptor);
			exit(0);
		}
	}
}
