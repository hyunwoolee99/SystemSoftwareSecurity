#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main()
{
	//redirection file var.
	int fd_in, fd_out, status;

	//input file open
	fd_in = open("input", O_RDONLY);
	
	//error case
	if(fd_in < 0)
	{
		perror("Input file open");
		exit(-1);
	}

	//output file open
	fd_out = open("result", O_WRONLY | O_CREAT | O_TRUNC, 0644);

	//error case
	if(fd_out < 0)
	{
		perror("Output file open");
		exit(-1);
	}

	//file descriptor redirection
	dup2(fd_in, 0);
	dup2(fd_out, 1);

	//fork, execution program at child process
	pid_t pid;
	if((pid=fork())<0)
	{
		perror("Fork");
		exit(-1);
	}
	if(pid == 0)
	{
		int ret;
		ret = execvp("./printer", NULL);
		if(ret < 0)
		{
			perror("Execute");
			exit(-1);
		}
		exit(0);
	}
	while(wait(&status) != pid) continue;
	return 0;
}

