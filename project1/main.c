#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
	int fd_in, fd_out;
	fd_in = open("input", O_RDONLY);
	if(fd_in<0)
	{
		fprintf(stderr, "file1 open failed\n");
	}
	fd_out = open("result", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd_in<0)
	{
		fprintf(stderr, "file2 open failed\n");
	}
	dup2(fd_in, 0);
	dup2(fd_out, 1);
	pid_t pid=fork();
	if(pid == 0)
	{
		execvp("./printer", NULL);
		exit(0);
	}
	return 0;
}

