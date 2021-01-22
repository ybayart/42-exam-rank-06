#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct	s_client
{
	int					fd;
	int					id;
	struct s_client*	next;
}				t_client;

int		g_id;

int		sockfd;
int		max_fd;

void	clear_client(t_client* list)
{
	t_client*	tmp;

	while (list != NULL)
	{
		tmp = (*list).next;
		close((*list).fd);
		free(list);
		list = tmp;
	}
}

int		add_client(t_client** list, int fd)
{
	t_client*	new;
	t_client*	tmp;

	if ((new = malloc(sizeof(t_client))) == NULL || list == NULL)
	{
		if (list != NULL)
			clear_client(*list);
		write(2, "Fatal error\n", 12);
		close(sockfd);
		exit(1);
	}
	(*new).fd = fd;
	(*new).id = g_id++;
	(*new).next = NULL;
	if ((*list) == NULL)
		(*list) = new;
	else
	{
		tmp = (*list);
		while ((*tmp).next != NULL)
			tmp = (*tmp).next;
		(*tmp).next = new;
	}
	return ((*new).id);
}

int		remove_client(t_client** list, int fd)
{
	t_client*	prev;
	t_client*	tmp;
	int			id;

	if (list == NULL)
	{
		write(2, "Fatal error\n", 12);
		close(sockfd);
		exit(1);
	}
	id = -1;
	prev = NULL;
	tmp = (*list);
	if (tmp != NULL && (*tmp).fd == fd)
	{
		(*list) = (*tmp).next;
		id = (*tmp).id;
		close((*tmp).fd);
		free(tmp);
	}
	else
	{
//		tmp = (*tmp).next;
		while (tmp != NULL && (*tmp).fd != fd)
		{
			prev = tmp;
			tmp = (*tmp).next;
		}
		if (tmp != NULL)
		{
			(*prev).next = (*tmp).next;
			close((*tmp).fd);
			id = (*tmp).id;
			free(tmp);
		}
	}
	return (id);
}

void	send_all(t_client* list, char* str, int fd)
{
	size_t	len;

	len = strlen(str);
	while (list != NULL)
	{
		if ((*list).fd != fd)
			send((*list).fd, str, len, 0);
		list = (*list).next;
	}
	write(1, str, len);
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

void	init_fdset(fd_set* set, t_client* list)
{
	FD_ZERO(set);
	max_fd = sockfd;
	while (list != NULL)
	{
		FD_SET((*list).fd, set);
		if (max_fd < (*list).fd)
			max_fd = (*list).fd;
		list = (*list).next;
	}
	FD_SET(sockfd, set);
}

int main(int argc, char** argv) {
	int connfd, ret, id, port;
	socklen_t	len;
	struct sockaddr_in servaddr, cli; 
	fd_set	set_read;
	struct timeval	timeout;
	t_client*	clients;
	t_client*	tmp;
	char	*buff;
	char	str[4200];
	char*	msg;
	ssize_t	size;

	if (argc != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
	{
		write(2, "Fatal error\n", 12); 
		exit(1);
	} 
	bzero(&servaddr, sizeof(servaddr)); 

	port = atoi(argv[1]);
	if (port <= 0)
	{
		write(2, "Fatal error\n", 12); 
		close(sockfd);
		exit(1);
	}
	// assign IP, PORT
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{ 
		write(2, "Fatal error\n", 12); 
		close(sockfd);
		exit(1);
	} 
	if (listen(sockfd, 10) != 0)
	{
		write(2, "Fatal error\n", 12); 
		close(sockfd);
		exit(1);
	}
	if ((clients = malloc(sizeof(t_client))) == NULL)
	{
		write(2, "Fatal error\n", 12); 
		close(sockfd);
		exit(1);
	}
	if ((buff = malloc(4096)) == NULL)
	{
		write(2, "Fatal error\n", 12);
		close(sockfd);
		free(clients);
		exit(1);
	}
	len = sizeof(cli);
	clients = NULL;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	g_id = 0;
	while (42)
	{
//		write(1, "loop\n", 5);
		init_fdset(&set_read, clients);
		ret = select(max_fd + 1, &set_read, NULL, NULL, &timeout);
		if (ret > 0)
		{
			if (FD_ISSET(sockfd, &set_read)) // New connection
			{
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				if (connfd >= 0)
				{
					id = add_client(&clients, connfd);
					if (max_fd < connfd)
						max_fd = connfd;
//					init_fdset(&set_read, clients);
					sprintf(str, "server: client %d just arrived\n", id);
					send_all(clients, str, connfd);
				}
			}
			else
			{
				tmp = clients;
				while (tmp != NULL)
				{
					connfd = (*tmp).fd;
					id = (*tmp).id;
					tmp = (*tmp).next;
					if (FD_ISSET(connfd, &set_read))
					{
						size = recv(connfd, buff, 4096, 0);
						if (size == 0) // Disconnected
						{
							id = remove_client(&clients, connfd);
							if (id != -1)
							{
//								init_fdset(&set_read, clients);
								sprintf(str, "server: client %d just left\n", id);
								send_all(clients, str, connfd);
							}
						}
						else if (size > 0)
						{
							msg = NULL;
							while (extract_message(&buff, &msg))
							{
								sprintf(str, "client %d: %s", id, msg);
								send_all(clients, str, connfd);
							}
						}
					}
				}
			}
		}
	}
}
