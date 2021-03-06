#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

//에러 출력
void error(char* msg)
{
	perror(msg);
	exit(1);
}

// 파일 전송을 위해 파일을 버퍼에 담아서 리턴
char *processing_File(char *filename)
{
	FILE *realfile;
	long fileSize;
	char *buffer;
	size_t result;
	if((realfile = fopen(filename,"rb"))==NULL)
		error("Error in reading file");
	fseek(realfile,0,SEEK_END);
	fileSize=ftell(realfile); //  ??
	rewind(realfile); //   ???
	buffer=(char*)malloc(sizeof(char)*fileSize);
	if(buffer == NULL)
		error("Memory error");
	result=fread(buffer,1,fileSize,realfile);
	if(result != fileSize)
		error("Reading error");
	fclose(realfile);
	return buffer;
}

//문자열의 길이 리턴, 만약 존재하지않는 파일의 경우 정해진 문자열의 길이인 21 리턴
int file_size(char *filename)
{
	FILE *realfile;
	if((realfile = fopen(filename,"rb"))==NULL)
		return 21;
	fseek(realfile,0,SEEK_END);
	return (ftell(realfile));
}

//주소로 입력받은 파일이 존재하는 지, 존재하면 processing_File함수로, 없으면 정해진 문자열 출력
char *check_file(char *filename)
{
	FILE *realfile;
	long fileSize;
	if((realfile = fopen(filename,"rb"))==NULL)
	{
		printf("\n%s\nThere is no such file",filename);
		fclose(realfile);
		return "There is no such file";
	}
	else
	{
		fclose(realfile);
		return processing_File(filename);
	}
}

//문자열에서 확장자만 리턴해주는 함수
char *extension(char *string)
{
	if(strlen(string)==0)
		return "";
	int i,j;
	for(i=0;i<strlen(string);i++)
		if(string[i]=='.')
			break;
	int ext_len=10-(i+1);
	char newString[ext_len+1]; //5-2 = 3
	for(j=0;j!=ext_len;j++)
	{
		newString[j]=string[i+1];
		i++;
	}
	newString[ext_len]='\0';
	printf("\n%c\n",newString[ext_len]);
	return newString;
}

int main(int argc, char *argv[])
{
	// 포트넘버를 입력했는지 검사
	if(argc<2)
		error("Please input a port number");
	int sockfd,newfd,portnum,rw,i,j;
	struct sockaddr_in server_addr,clients_addr;
	char buffer[512],sendfile[100];
	socklen_t clientnum;
	portnum = atoi(argv[1]);
	// 소켓 생성 및 검사 
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1)
		error("Fail in Creating a socket");
    bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portnum);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	// bind 
	if(bind(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr))==-1)
		error("Fail to bind");
	// listen 
	if(listen(sockfd,10)==-1)
		error("listen fail");
	while(1)
	{
		clientnum=sizeof(clients_addr);
		// accept 
		newfd=accept(sockfd,(struct sockaddr *)&clients_addr,&clientnum);
		if(newfd==-1)
			error("Fail to accept clients");
		bzero(buffer,512);
		bzero(sendfile,100);
		// read 
		rw=read(newfd,buffer,511);
		if(rw<=0)
			error("error in reading");
		// 터미널에 client로부터 받은 메시지 출력 
		printf("%s\n",buffer);
		//  client가 입력한 IP주소:포트넘버/~에서 ~를 sendfile에 입력 
		for(i=5,j=0;buffer[i]!=buffer[3];i++,j++)
			sendfile[j]=buffer[i];
		/*
		if((strcmp(sendfile,"")!=0)&&(strcmp(extension(sendfile),"html")!=0))
			if(send(newfd,check_file(sendfile),file_size(sendfile),0)==-1)
				error("error in sending");
		*/
		// client의 ~가 안적혀있는 경우 index.html로 이동
		if(strcmp(sendfile,"")==0)
		{
			bzero(sendfile,100);
			char *str="index.html";
			for(i=0;i<10;i++)
				sendfile[i]=str[i];
			j=10;
		}
		// write
		if(j>1)
		{
			rw=write(newfd,check_file(sendfile),file_size(sendfile));
			// write가 안된 경우
			if(rw<=0)
				error("error in writing");
		}
		else
			rw=write(newfd,"I receive your message",22);
		close(newfd);
	}
	close(sockfd);
	return 0;
}