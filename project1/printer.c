#include <stdio.h>
#define MAX_INPUT_LENGTH 255

int main()
{
	char input[MAX_INPUT_LENGTH];
	while(fgets(input, MAX_INPUT_LENGTH, stdin) != NULL)
	{
		fprintf(stdout, "%s", input);
	}
	return 0;
}

