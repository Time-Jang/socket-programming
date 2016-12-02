#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <arpa/inet.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#define read_buff_size 1024*100
#define BACKLOG 10
#define MAX_CACHE_SIZE 5*1024*1024
#define MAX_OBJECT_SIZE 512*1024
#define http_size 100
#define file_len 100
int cache_max_size;

void error(char* msg)
{
	perror(msg);
	exit(1);
}
//에러를 출력해주고 프로그램 종료

int is_time(char *buf)
{
	int i;
	int j = 0;
	for(i = 0; i < 14; i++)
	{
		if(isdigit(buf[i]))
			j++;
		else
			return 0;
	}
	if(buf[i] == '\n')
		j++;
	if(j == 15)
		return 1;
	return 0;
}//해당 문자열이 시간인지 검사

int check_http(char *http)
{
	int i,j;
	if(strncmp(http,"http://",7) == 0)
		return 7;
	if(strncmp(http,"https://",8) == 0)
		return 8;
	return 0;
} //해당 문자열이 http인지 https인지 봐서 domain이 어디부터 시작하는 지 알려주는 함수

int http_parse(char *string,char *http)
{
	int i,j,k;
	for(k = 0; string[k] != ' '; k++)
		;
	for(i = k+1, j = 0; string[i] != ' '; i++,j++)
		http[j] = string[i];
	return 1;
} //문자열을 받아서 포인터를 통해 GET부분등을 제외하고 사용자가 입력한 주소만 얻는다.

int http_parse_file(char *http, char *file)
{
	int i,j,k;
	j = check_http(http);
	for(k = j; http[k] != '/'; k++)
		;
	strncpy(file,&http[k],(int)strlen(http)-k);
	return 1;
}//사용자가 브라우저에 입력한 주소에서 file에 해당하는 부분만 포인터에 저장

int http_parse_domain(char *http, char *domain)
{
	int i,j,k,start_num;
	start_num = check_http(http);
	for(k = start_num; http[k] != ':'; k++)
	{
		if(k > (int)strlen(http))
			break;
		if(http[k] == '/')
			break;
	}
	for(i = start_num, j = 0; i < k; i++,j++)
		domain[j] = http[i];
	return 1;
}//사용자가 브라우저에 입력한 주소에서 파일과 http부분을 제외한 도메인 부분만 포인터에 저장

void cache_domain_write(char *http)
{
	FILE *file;
	char *string = "Domain:";
	char domain_char[http_size];
	char buffer[http_size+(int)strlen(string)];

	bzero(domain_char,http_size);
	http_parse_domain(http,domain_char);
	file = fopen("cache", "a+b");
	if(file == NULL)
		error("cache_domain_write fail");
	strcpy(buffer,string);
	strcpy(&buffer[(int)strlen(string)],domain_char);
	fwrite(buffer, 1, (int)strlen(buffer), file);
	fwrite("\n", 1, 1, file);
	fclose(file);
}//사용자가 domain을 입력했을때 캐시에 그 해당 도메인이 있는지 검사하기위해 캐시에 도메인을 쓰는 함수

int cache_read(char *http)
{
	FILE *file;
	char *string = "Domain:";
	char strTemp[255],*pStr,domain_char[http_size];
	char buffer[http_size+(int)strlen(string)];
	
	bzero(domain_char,http_size);
	http_parse_domain(http,domain_char);
	strcpy(buffer,string);
	strcpy(&buffer[(int)strlen(string)],domain_char);
	strcpy(&buffer[(int)strlen(buffer)],"\n");
	
	file = fopen("cache", "r");
	if(file == NULL)
		error("cache_remove fail");
	while(!feof(file))
	{
		pStr = fgets(strTemp,sizeof(strTemp),file);
//		printf("\n\nstrTemp : %s\nbuffer : %s\n",strTemp,buffer);
//		printf("%d == %d ? %d",(int)strlen(strTemp),(int)strlen(buffer),(int)strlen(strTemp) == (int)strlen(buffer));
		if((int)strlen(strTemp) == (int)strlen(buffer))
		{
// 			printf("YES??????????????????????????\n");
			if(strcmp(strTemp,buffer) == 0)
			{
//				printf("YES!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				return 1;
			}
//				return 1;
		}	
	}
	fclose(file);
	return 0;
}//캐시를 읽어들인다.

void search_obj(char *http,int newfd)
{
	FILE *file;
	char *string = "Domain:";
	char *string2 = "HTTP/1.";
	char strTemp[255],*pStr,domain_char[http_size];
	char buffer[http_size+(int)strlen(string)];
	char buffer2[read_buff_size];
	int rw;
	int on = 0;
	int off = 0;
	
	bzero(domain_char,http_size);
	bzero(buffer2,read_buff_size);
	
	http_parse_domain(http,domain_char);
	strcpy(buffer,string);
	strcpy(&buffer[(int)strlen(string)],domain_char);
	
	file = fopen("cache", "r");
	if(file == NULL)
		error("search_obj fail");
	while( !feof(file))
	{
		pStr = fgets(strTemp,sizeof(strTemp),file);
		if(strlen(strTemp) == strlen(buffer))
		{
			if(!strcmp(strTemp,buffer))
			{
				on = 1;
				off = 1;
			}
			else if(!strncmp(string,strTemp,7))
				break;
		}
		if(on == 1)
		{
			if(!strncmp(string2,strTemp,7))
			{
				if(off != 1)
				{
					rw = write(newfd,buffer2,(int)strlen(buffer2));
					off = 1;
				}
				bzero(buffer2,read_buff_size);
				off = 0;
				strcpy(buffer2,strTemp);
			}
			else
			{
				strcpy(&buffer2[(int)strlen(buffer2)],strTemp);
				off = 0;
			}
			
		}
		bzero(strTemp,255);
	}
}//캐시에 해당 패킷이 있는지 검사

void cache_LRU()
{
	FILE *file;
	char strTemp[255],temp[50];
	char *string = "Domain:";
	char *https = "HTTP/1.";
	char *pStr;
	int i = 0;
	int count = 0;
	int j = 0;
	int k = 0;
	file = fopen("cache", "r");
	
	while( !feof(file))
	{
		bzero(strTemp,255);
		pStr = fgets(strTemp,sizeof(strTemp),file);
		if(is_time(strTemp))
			count++;
	}
	bzero(strTemp,255);
	fclose(file);
	
	printf("count : %d\n",count);
	
	char *buffer[count][3];
	buffer[0][0] = (char *)malloc(sizeof(char)*50);
	buffer[0][1] = (char *)malloc(sizeof(char)*16);
	buffer[0][2] = (char *)malloc(sizeof(char)*read_buff_size);
	strcpy(buffer[0][1],"33333333333333");
	strcpy(buffer[0][0],"afafas");
	
	file = fopen("cache", "r");
	while( !feof(file))
	{
		pStr = fgets(strTemp,sizeof(strTemp),file);
		if(!strncmp(strTemp,string,7))
			strcpy(temp,strTemp);
		if(is_time(strTemp))
		{
			i++;
			buffer[i][0] = (char *)malloc(sizeof(char)*50);
			buffer[i][1] = (char *)malloc(sizeof(char)*16);
			buffer[i][2] = (char *)malloc(sizeof(char)*(read_buff_size+100));
			strcpy(buffer[i][0],temp);
			strcpy(buffer[i][1],strTemp);
		}
		else
			if(strncmp(strTemp,string,7))
				strcpy(&buffer[i][2][(int)strlen(buffer[i][2])],strTemp);
		bzero(strTemp,strlen(strTemp));
	}
	fclose(file);
	
	for(j = 1; j < count; j++)
		if(atol(buffer[j][1]) < atol(buffer[k][1]))
			k = j;
	
	file = fopen("cache", "w+b");
	
	strcpy(temp,"abcabc");
	for(j = 1; j < count; j++)
	{
		if(j == k)
			continue;
		if(j == 1)
			fwrite(buffer[j][0], 1, (int)strlen(buffer[j][0]), file);
		if(j != 1 && !strcmp(buffer[j][0],buffer[j-1][0]))
			fwrite(buffer[j][0], 1, (int)strlen(buffer[j][0]), file);
		fwrite(buffer[j][1], 1, (int)strlen(buffer[j][1]), file);
		fwrite(buffer[j][2], 1, (int)strlen(buffer[j][2]), file);
	}
	
	fclose(file);

	for(i = 0; i < count; i++)
		for(j = 0; j < 2; j++)
			free(buffer[i][j]);
} //캐시가 꽉 찼을때 덜 사용된 부분을 삭제하는 함수

void cache_check()
{
	FILE *file;
	file = fopen("cache", "a+b");
	if(file == NULL)
		error("cache_check fail");
	fseek(file, 0L, SEEK_END);
	cache_max_size = ftell(file);
	fclose(file);
} //캐시의 크기가 몇인지 알려주는 함수

void cache(char *buffer, int size)
{
	if(size > MAX_OBJECT_SIZE)
		return;
	cache_check();
	if(cache_max_size > MAX_CACHE_SIZE)
		cache_LRU();
	FILE *cache;
	struct tm *tm;
	time_t time_var;
	char buffer2[16];
	
	cache = fopen("cache", "a+b");
	if(cache == NULL)
		error("caching fail");
	
	time_var = time(NULL);
	tm = (struct tm*)localtime(&time_var);
	
	bzero(buffer2,16);
	
	sprintf(buffer2,"%04d%02d%02d%02d%02d%02d\n",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	
	fwrite(buffer2, 1, 16, cache);
	fwrite(buffer, 1, size, cache);
	fwrite("\n", 1, 1, cache);
	fclose(cache);
}// 해당 패킷을 캐시에 시간과 함께 기록하거나 캐시를 비우는 함수

int find_port(char *http)
{
	int i,j;
	char port[5];
	for(i = 0,j = 0; i < strlen(http); i++)
	{
		if(http[i] == ':')
			if(http[i+1] != '/')
				j++;
			
		if(j > 0 && http[i] == '/')
			break;
		if(j > 0)
		{
			port[j-1] = http[i+1];
			j++;
		}
	}
	if(j == 0)
		return 80;
	return atoi(port);
}//사용자가 포트를 입력했을 경우 해당부분을 리턴, 만약 입력이 없으면 기본값인 80

void Logging(char *domain, char *ip, int size)
{
	FILE *log;
	char *current_time;
	char buffer[100], buffer2[1024];
	struct tm *tm;
	time_t time_var;
	log = fopen("proxy.log", "a+b");
	if(log == NULL)
		error("log open fail");
	
	bzero(buffer,100);
	bzero(buffer2,1024);
	
	time_var = time(NULL);
	tm = (struct tm*)localtime(&time_var);
	current_time = (char *)asctime(tm);
	
	strncpy(buffer, current_time, strlen(current_time) - 1);
	
	sprintf(buffer2, "%s: %s %s %d\n", buffer, ip, domain, size);
	fwrite(buffer2, 1, 1024, log);
	fclose(log);
}//패킷을 로깅하는 함수

int target_connect(char *http, int newfd)
{
	if((int)strlen(http) <= 10)
		return 0;
	int check = 0;
	check = cache_read(http);
	if(check == 0)
	{
		cache_domain_write(http);
		int sockfd,portnum,rw,i,j,header_len,size;
		struct sockaddr_in target_addr;
		struct hostent *domain;
		char buffer[512],domain_char[http_size],file[file_len],buffer_read[read_buff_size];
		socklen_t clientnum;
		portnum = find_port(http);
		// 소켓 생성 및 검사 
		if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1)
			error("Fail in Creating a socket 11");
	
	    bzero((char *) &target_addr, sizeof(target_addr));
		bzero(buffer,512);
		bzero(domain_char,http_size);
		bzero(file,file_len);
		bzero(buffer_read,read_buff_size);
	
		http_parse_file(http,file);
		http_parse_domain(http,domain_char);
		printf("port : %d\n",portnum);
		printf("domain : %s\n",domain_char);
		printf("file : %s\n",file);	
	
		domain = gethostbyname(domain_char);
		if(domain == NULL)
			error("gethostbyname 11");
	
		target_addr.sin_family = AF_INET;
		target_addr.sin_port = htons(portnum);
		memcpy(&target_addr.sin_addr, domain->h_addr_list[0], domain->h_length);
	
		if(connect(sockfd, (struct sockaddr*)&target_addr, sizeof(struct sockaddr)) == -1)
			error("connect 11");
	
		sprintf(buffer,"GET %s HTTP/1.0\r\n\n",file);
	
		printf("proxy_write\n%s\n",buffer);
	
		rw = write(sockfd,buffer,(int)strlen(buffer));
		if (rw<=0)
			error("write 11");
	
		size = 0;
		bzero(buffer_read,read_buff_size);
	
		printf("proxy_read\n");
	
		while((rw = read(sockfd,buffer_read,read_buff_size)) > 0)
		{
			size = size + (int)strlen(buffer_read);
			rw = write(newfd,buffer_read,(int)strlen(buffer_read));
			cache(buffer_read, (int)strlen(buffer_read));
			if(rw < 0)
				error("write 12");
	//		printf("===================================\n");
			printf("\n%s\n",buffer_read);
		
			bzero(buffer_read,strlen(buffer_read));
		}
		printf("IP : %s\n",inet_ntoa(*(struct in_addr*)domain->h_addr_list[0]));
		Logging(domain_char, inet_ntoa(*(struct in_addr*)domain->h_addr_list[0]), size);
	
		close(sockfd);
		return 1;
		
	}
	else
	{
		search_obj(http,newfd);
		return 1;
	}
}//사용자가 브라우저에 주소를 입력했을 경우 해당 주소의 서버와 프록시의 소켓연결을 해주는 함수

void client_connect(int portnum)
{
	int sockfd,newfd,rw,i,count;
	struct sockaddr_in proxy_addr,clients_addr;
	char buffer[512],http[http_size];
	socklen_t clientnum;
	// 소켓 생성 및 검사 
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1)
		error("Fail in Creating a socket 00");
    bzero((char *) &proxy_addr, sizeof(proxy_addr));
	bzero(http,http_size);
	proxy_addr.sin_family = AF_INET;
	proxy_addr.sin_port = htons(portnum);
	proxy_addr.sin_addr.s_addr = INADDR_ANY;
	// bind 
	if(bind(sockfd,(struct sockaddr *)&proxy_addr,sizeof(proxy_addr))==-1)
		error("Fail to bind 00");
	// listen 
	if(listen(sockfd,10)==-1)
		error("listen fail 00");
	i = 1;
	count = 0;
	while(i == 1)
	{
		printf("start accept\n");
		clientnum=sizeof(clients_addr);
		// accept 
		newfd=accept(sockfd,(struct sockaddr *)&clients_addr,&clientnum);
		if(newfd==-1)
			error("Fail to accept clients 00");
		bzero(buffer,512);
		// read 
		printf("start read\n");
		rw=read(newfd,buffer,511);
		if(rw<0)
			error("error in reading 00");
		// 터미널에 client로부터 받은 메시지 출력 
		printf("browser_read\n%s\n%d\n",buffer,(int)strlen(buffer));
		if(!(int)strlen(buffer))
			break;
		//  client가 입력한 IP주소:포트넘버/~에서 ~를 sendfile에 입력
		http_parse(buffer,http);
		printf("address : %s\n",http);
		i = target_connect(http, newfd);
		printf("===================================\n");
		bzero(http,http_size);
		printf("%d\n===================================\n",++count);
	}
	close(newfd);
	close(sockfd);
}//프록시와 프록시 사용자의 연결을 해주는 함수

int main(int argc,char *argv[])
{
	// 포트넘버를 입력했는지 검사
	if(argc != 2)
		error("Please input only a port number");
	if(atoi(argv[1]) < 1024 || atoi(argv[1]) > 65536)
		error("Please input a port number between 1024 to 65536");
	cache_check(); //프록시 시작시 프록시의 캐시의 크기가 몇인지 검사한다.
	
	client_connect(atoi(argv[1]));
	
	return 0;
}