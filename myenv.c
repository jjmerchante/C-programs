/**
* myenv.c
*
* Copyright Jose J. Merchante 
* jjmerchante@gmail.com
*
* Guiven a list of environment variables as argument,
* returns the variable name with its value.
* Also prints the UID and the PID where the process is
* running
*
* Compile: ggc -o myenv myenv.c
* Run: ./myenv HOME SHELL PATH PEPE...
**/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


static void
print_variables (char **array_var, int num_var){
	int k = 0;
	char *valor_var;

	for (k = 1; k< num_var; k++){
		valor_var = getenv(array_var[k]);
		if (valor_var == NULL){
			fprintf (stderr, "error: %s does not exist\n", array_var[k]);
		} else {
			printf("%s: %s\n", array_var[k], valor_var);
		}
	}	
}

int
main (int argc, char *argv[]){
	
	printf("UID: %d\n", getuid());
	printf("PID: %d\n", getpid());
	print_variables (argv, argc);
	
	
	
	exit(0);
	
}