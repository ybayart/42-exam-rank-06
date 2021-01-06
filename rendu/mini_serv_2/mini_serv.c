#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
			newbuf = malloc(sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == NULL)
				return (-1);
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
	if (newbuf == NULL)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

int main(int argc, char** argv) {
	int sockfd, connfd, max_fd, ret;
	socklen_t	len;
	struct timeval timeout;
	struct sockaddr_in servaddr, cli; 
	fd_set	set_read, set_write;
	char	buff[4096];
	ssize_t	ssize_len;

	if (argc != 2)
	{
		write(1, "Wrong number of arguments\n", 26); 
		exit(1);
	}
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
	{
		write(1, "Fatal error\n", 12); 
		exit(1);
	} 
	bzero(&servaddr, sizeof(servaddr)); 
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{ 
		write(1, "Fatal error\n", 12); 
		exit(1);
	} 
	if (listen(sockfd, 10) != 0)
	{
		write(1, "Fatal error\n", 12); 
		exit(1);
	}
	len = sizeof(cli);
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	FD_ZERO(&set_read);
	FD_ZERO(&set_write);
	FD_SET(sockfd, &set_read);
	max_fd = sockfd;
	bzero(&buff, 4096);
	connfd = -1;
	while (42)
	{
//		write(1, "loop\n", 5);
		ret = select(max_fd + 1, &set_read, &set_write, NULL, &timeout);
		if (ret > 0)
		{
//			write(1, "here\n", 5);
			if (FD_ISSET(sockfd, &set_read)) // new connection
			{
//				write(1, "bzzz\n", 5);
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				if (connfd < 0) { 
					write(1, "Fatal error\n", 12); 
					exit(1);
				} 
				FD_SET(connfd, &set_read);
				max_fd = (max_fd < connfd ? connfd : max_fd);
				write(1, "connected\n", 10);
			}
			else if (connfd != -1 && FD_ISSET(connfd, &set_read))
			{
				ssize_len = recv(connfd, &buff, 4096, 0);
				if (ssize_len <= 0) // close connection
				{
					FD_ZERO(&set_read);
					FD_SET(sockfd, &set_read);
					max_fd = sockfd;
					close(connfd);
					connfd = -1;
					write(1, "disconnected\n", 13);
				}
				else
					write(1, buff, ssize_len);
			}
		}
	}
}
