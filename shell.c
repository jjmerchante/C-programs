/**
* shell.c
*
* Copyright Jose J. Merchante 
* jjmerchante@gmail.com
*
* Just a simple shell for Linux
*
* Compile: ggc -o shell shell.c
* Run: ./shell
**/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <err.h>


#define PROMPT "$ "
#define MAX_LONG_LINE 500
#define MAXTOKENS 200
#define MAXCMDS 100

struct setoftokens
{
	char *tokens[MAXTOKENS];
	int ntokens;
};
typedef struct setoftokens setoftokens;

struct command
{
	char *cmd_path;
	char **argv_cmd;
	int fdin;
	int fdout;
};
typedef struct command command;

struct setofcommands
{
	command *arraycmds;
	int ncmds;
	int wait_cmds;
};
typedef struct setofcommands setofcommands;

//Trocea una string por sep y devuelve el número de tokens
//y los tokens
static int
ntokenize(int maxtoks, char *string, char **toks, char *sep){
	char *str = string;
	int j;
	char *token;
	char *saveptr;
	int num_tokens= 0;

	for (j = 0; num_tokens< maxtoks; j++) {
		token = strtok_r(str, sep, &saveptr);
		if (token == NULL){
			break;
		} else {
			toks[j]=token;
			num_tokens++;
		}
		str = NULL;
	}
	return num_tokens;
}

//Lo último introducido es &
static int
end_ampersand (setoftokens cmdstok){
	char *last_tok = cmdstok.tokens[cmdstok.ntokens-1];
	char last_char = last_tok[strlen (last_tok)-1];
	if (last_char == '&')
		return 1;
	else
		return 0;
}

//Parte un token en subtokens y devuelve el primero
static char *
getword (char *words){
	char *tokens[3];
	int num_toks;

	num_toks=ntokenize (2, words, tokens, " \n\t><|&");
	if (num_toks == 0)
		tokens[0]=NULL;
	return tokens[0];
}

//Devuelve la palabra que haya tras aparecer un caracter
//y antes de que aparezca otro caracter especial de shell
static char *
getnextwordto (setoftokens cmdstok, char *wrd){
	char **array_tok =cmdstok.tokens;
	int num_toks = cmdstok.ntokens;
	int i;
	char tok_aux[150];
	int n_subtkns;
	char *subtkns[2];
	char *wrd_aux=NULL;
	char *wrd_out=NULL;

	for (i=0; i< num_toks; i++){
		strncpy (tok_aux, array_tok[i], 100);
		n_subtkns = ntokenize (2, tok_aux, subtkns, wrd);
		if (n_subtkns == 0) {
			wrd_aux = getword (array_tok[i+1]);
			break;
		}if (n_subtkns == 2){
			wrd_aux = getword (subtkns[1]);
			break;
		}else if (strlen (subtkns[0])!=strlen(array_tok[i])){
			if (array_tok[i][0]==wrd[0]){
				wrd_aux = getword (subtkns[0]);
				break;
			} else {
				wrd_aux = getword (array_tok[i+1]);
				break;
			}
		}
	}
	if (wrd_aux != NULL){
		wrd_out = strdup (wrd_aux);
	}
	return wrd_out;
}

//Devuelve el descriptor de fichero del redireccionamiento
//de entrada
static int 
getin (setoftokens cmdstok){
	int fdin=0;
	char *fich_in;

	fdin = 0;
	fich_in = getnextwordto (cmdstok, "<");
	if (fich_in != NULL){
		fdin = open (fich_in, O_RDONLY);
		if (fdin<0){
			fprintf(stderr, "You can't use %s\n", fich_in);
			return -1;
		}
		free (fich_in);
	} else if (end_ampersand(cmdstok)){
		fdin = open ("/dev/null", O_RDONLY);
		if (fdin<0){
			fprintf(stderr, "You can't use &\n");
			return -1;
		}
	}
	return fdin;
}

//Devuelve el redireccionamiento de salida
static int
getout (setoftokens cmdstok){
	int fdout;
	char *fich_out;

	fdout = 1;
	fich_out = getnextwordto (cmdstok, ">");
	if (fich_out != NULL){
		fdout = open (fich_out, O_WRONLY|O_CREAT|O_TRUNC, 0660);
		if (fdout<0){
			fprintf(stderr, "You can't use %s\n", fich_out);
			return -1;
		}
		free (fich_out);
	}
	return fdout;
}

//Cambia el directorio de trabajo
static int
chang_dir (command *builtin){
	char *dir_dest;

	if (builtin->argv_cmd[1] == NULL) {
		if ((dir_dest = getenv("HOME"))==NULL){
			fprintf(stderr, "You cannot access HOME\n");
			return -1;
		}
	} else {
		dir_dest = builtin->argv_cmd[1];
	}
	if (chdir(dir_dest)!=0){
		fprintf(stderr, "You cannot access %s\n", dir_dest);
		return -1;
	}
	return 0;
}

static int
get_uid (command *builtin){
	int uid;

	uid = getuid ();
	printf("%d\n", uid);
	return 0;
}

//Ejecuta un built in
static int
exec_builtin (command *builtin){
	if (strcmp (builtin->cmd_path, "cd")==0){
		return chang_dir (builtin);
	} else if (strcmp (builtin->cmd_path, "myuid")==0){
		return get_uid (builtin);
	} else {
		fprintf(stderr, "Built in not found\n");
		return -1;
	}
	return 0;
}

//Devuelve si el comando es built in o no
static int
is_builtin (char *cmd){
	if (strcmp (cmd, "cd")==0){
		return 1;
	}else if (strcmp (cmd, "myuid")==0){
		return 1;
	}else{
		return 0;
	}
}

//Devuelve si un comando está en el directorio actual
static int
is_inthisdir (char *cmd){	
	if (!access(cmd, X_OK)){
		return 1;
	} else {
		return 0;
	}
}

//borra todos los descriptores de fichero del array de comandos
//-1 para borrar todos
static int
close_all (setofcommands *cmds){
	int n;
	command *cmd_aux;

	for (n=0;n<(cmds->ncmds);n++){
		cmd_aux = &(cmds->arraycmds[n]);
		if (cmd_aux->fdin != 0 && cmd_aux->fdin != 1){
			if (close (cmd_aux->fdin)<0){
				fprintf(stderr, "error closing in\n");
				return -1;
			}
		}
		if (cmd_aux->fdout != 0 && cmd_aux->fdout != 1){
			if (close (cmd_aux->fdout)<0){
				fprintf(stderr, "error closing out\n");
				return -1;
			}
		}
	}
	return 0;
}

//Ejecuta los comandos
static int
exec_cmds (setofcommands *cmds){
	int n;
	int sts, pid;
	command *cmd_aux;

	for (n=0; n<(cmds->ncmds);n++){
		pid = fork ();
		switch(pid){
		case -1:
			fprintf(stderr, "fork failed\n");
			return -1;
		case 0:
			cmd_aux = &(cmds->arraycmds[n]);
			dup2(cmd_aux->fdin, 0);
			dup2(cmd_aux->fdout, 1);
			if (close_all(cmds)<0){
				exit(1);
			}
			execv (cmd_aux->cmd_path, cmd_aux->argv_cmd);
			err (1,"%s\n" ,cmd_aux->cmd_path);
		}
	}
	close_all(cmds);
	if (cmds->wait_cmds){
		while (wait(&sts) != pid)
			;
	}
	return sts;
}

//Añade el fd de entrada y salida a cada comando
static void
getinoutcmds (int fdin, int fdout, setofcommands *cmds){
	int i;
	int fd[2];
	command *cmd_aux;

	for (i=0;i<(cmds->ncmds)-1; i++){
		pipe (fd);
		cmd_aux = &(cmds->arraycmds[i]);
		cmd_aux->fdout = fd[1];
		cmd_aux = &(cmds->arraycmds[i+1]);
		cmd_aux->fdin = fd[0];
	}
	cmd_aux = &(cmds->arraycmds[0]);
	cmd_aux->fdin = fdin;
	cmd_aux = &(cmds->arraycmds[cmds->ncmds-1]);
	cmd_aux->fdout = fdout;
}

//Obtiene el path de un comando y devuelve el path con el
//comando concatenado
static char *
get_path_cmd (char *cmd){
	char *path_vect[25];
	int num_elem_path;
	int i;
	char path[500]; //path para no machacar getenv("PATH")
	int bufsize = 200;
	char *path_aux = (char *)malloc (bufsize);

	strncpy (path, getenv("PATH"), 500);
	num_elem_path = ntokenize (25, path, path_vect, ":");
	
	for (i = 0; i<num_elem_path; i++){
		snprintf(path_aux, bufsize, "%s/%s", path_vect[i], cmd);
		if (!access(path_aux, X_OK)){
			return path_aux;
		}
	}
	return NULL;
}

static char *
get_place_cmd (char *cmd){
	char *cmd_out = NULL;

	if (is_builtin(cmd)){
		cmd_out = cmd;
	} else if (is_inthisdir(cmd)) {
		cmd_out = cmd;
	} else {
		cmd_out = get_path_cmd (cmd);
	}

	return cmd_out;
}

//Libera memoria creada en cmds
static void
free_cmds (setofcommands *cmds){
	int i;
	command *cmd_aux;

	if (cmds->arraycmds != NULL){
		for (i=0; i<(cmds->ncmds);i++){
			cmd_aux = &(cmds->arraycmds[i]);
			if (cmd_aux->cmd_path != NULL){
				free (cmd_aux->cmd_path);
				cmd_aux->cmd_path = NULL;
			}
			if (cmd_aux->argv_cmd != NULL){
				free (cmd_aux->argv_cmd);
				cmd_aux->argv_cmd = NULL;
			}
		}
	}
	free(cmds->arraycmds);
	cmds->arraycmds = NULL;
	free (cmds);
	cmds = NULL;
}

static void
save_cmd (command *cmd, char **argv, int argc){
	char **argvaux;
	int i;

	cmd->cmd_path = NULL;
	cmd->argv_cmd = malloc ((argc + 1) * sizeof(char *));
	argvaux = cmd->argv_cmd;

	for (i=0;i<argc;i++){
		argvaux[i] = malloc (sizeof(char *) * (strlen (argv[i])+1) );
		strncpy(argvaux[i], argv [i], strlen (argv[i])+1);
	}
	argvaux[argc]=NULL;
}

static setofcommands *
getcommands (setoftokens cmdstok){
	char *sub_tkns[100];
	int n_subtkns;
	int i;
	char *tok;
	command *cmd_aux;
	setofcommands *cmds;
	
	cmds = malloc(sizeof(cmds->ncmds)+ sizeof(cmds->arraycmds));
	cmds->ncmds = cmdstok.ntokens;
	cmds->arraycmds = malloc (cmds->ncmds * sizeof(command));

	for (i=0;i<(cmds->ncmds);i++){
		tok = cmdstok.tokens[i];
		//Si hubiera redireccion entre pipes o al final, se quita
		n_subtkns = ntokenize (100, tok, sub_tkns,"><&");
		tok =sub_tkns[0];

		n_subtkns = ntokenize (100, tok, sub_tkns,"  \t\n");
		cmd_aux = &(cmds->arraycmds[i]);
		save_cmd (cmd_aux, sub_tkns, n_subtkns);
		cmd_aux->cmd_path = get_place_cmd (cmd_aux->argv_cmd[0]);
		if (cmd_aux->cmd_path == NULL){
			fprintf(stderr, "%s: command not found\n", cmd_aux->argv_cmd[0]);
			free_cmds (cmds);
			return NULL;
		}
	}
	return cmds;
}

//Procesar los tokens de la linea de comandos
static int
process_tokens (setoftokens cmdstok){
	int fd_in, fd_out, wait;
	int sts_proc;
	setofcommands *cmds;
	command *cmd_aux;

	fd_in =getin (cmdstok);
	if (fd_in < 0){
		return -1;
	}
	fd_out =getout (cmdstok);
	if (fd_out<0){
		return -1;
	}

	wait = !end_ampersand(cmdstok);
	cmds = getcommands (cmdstok);
	if (cmds == NULL){
		return -1;
	}
	getinoutcmds (fd_in, fd_out, cmds);
	cmds->wait_cmds = wait;
	
	cmd_aux = &(cmds->arraycmds[0]);
	if (is_builtin (cmd_aux->cmd_path)){
		sts_proc = exec_builtin (cmd_aux);
		free_cmds (cmds);
	} else {
		sts_proc = exec_cmds (cmds);
		free_cmds (cmds);
	}
	return sts_proc;
}

int
main(int argc, char *argv[]){
	int line_length = MAX_LONG_LINE;
	char line_cmd [line_length];
	setoftokens linetok; 
	
	//Acaba con fin de fichero
	for (;;){
		printf("%s", PROMPT);
		fflush (stdout);
		if (fgets (line_cmd, line_length, stdin)==NULL){
			printf("exit\n");
			exit(0);
		}

		linetok.ntokens=ntokenize(MAXTOKENS,line_cmd,linetok.tokens,"|\n");
		if (linetok.ntokens==0){
			continue;
		} else if (linetok.ntokens==MAXTOKENS){
			fprintf(stderr, "Write less args. MAX: %d\n", MAXTOKENS-1);
			continue;
		}
		process_tokens (linetok);

	}
}