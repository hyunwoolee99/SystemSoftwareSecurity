#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
	//redirection file var.
	int fd_in, fd_out;

	//input file open
	fd_in = open("input", O_RDONLY);
	
	//error case
	if(fd_in < 0)
	{
		fprintf(stderr, "file1 open failed\n");
	}

	//output file open
	fd_out = open("result", O_WRONLY | O_CREAT | O_TRUNC, 0644);

	//error case
	if(fd_in < 0)
	{
		fprintf(stderr, "file2 open failed\n");
	}

	//file descriptor redirection
	dup2(fd_in, 0);
	dup2(fd_out, 1);

	//fork, execution program at child process
	pid_t pid=fork();
	if(pid == 0)
	{
		execvp("./printer", NULL);
		exit(0);
	}
	return 0;
}

