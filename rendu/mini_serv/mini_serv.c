#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

void	fatal_error(void)
{
	write(2, "Fatal error\n", 12);
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
			{
				fatal_error();
				return (-1);
			}
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

int main(int argc, char** argv) {
	int sockfd, connfd, port, max_fd, ret;
	socklen_t len;
	struct sockaddr_in servaddr, cli; 
	fd_set set_read, set_write;
	struct timeval timeout;
	char buf[4096];
	ssize_t len_ssize;

	if (argc != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	port = atoi(argv[1]);
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		fatal_error();
		exit(1); 
	} 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{ 
		fatal_error();
		exit(1); 
	} 
	if (listen(sockfd, 10) != 0)
	{
		fatal_error();
		exit(1); 
	}
	len = sizeof(cli);
	timeout.tv_sec = 10;
	FD_ZERO(&set_read);
	FD_ZERO(&set_write);
	FD_SET(sockfd, &set_read);
	max_fd = sockfd;
	while (42)
	{
		write(1, "toto\n", 5);
		ret = select(max_fd + 1, &set_read, &set_write, NULL, &timeout);
		if (ret > 0)
		{
			if (FD_ISSET(sockfd, &set_read))
			{
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				if (connfd < 0)
				{ 
					fatal_error();
					exit(1); 
				}
				max_fd = (max_fd < connfd ? connfd : max_fd);
				FD_SET(connfd, &set_read);
				write(1, "connection accepted\n", 20);
			}
			else if (FD_ISSET(connfd, &set_read))
			{
				len_ssize = recv(connfd, buf, 4096, 0);
				write(1, buf, len_ssize);
			}
//			printf("ret: %d\n", ret);
//			printf("sockfd: %d | %d\n", FD_ISSET(sockfd, &set_read), FD_ISSET(sockfd, &set_write));
//			printf("connfd: %d | %d\n", FD_ISSET(connfd, &set_read), FD_ISSET(connfd, &set_write));
		}
	}
}
