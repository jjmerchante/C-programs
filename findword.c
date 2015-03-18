/**
* findword.c
*
* Copyright Jose J. Merchante 
* jjmerchante@gmail.com
*
* Guiven a word and a set of directories, returns 
* the file names where the word is found at the begining
* Uses recursive search 
*
* Compile: ggc -o findword findword.c
* Run: ./myenv HOME SHELL PATH PEPE...
**/


#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>
#include <err.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

char *No_Entrar[]={".",".."};

//Comprobar si es directorio
static int
es_dir (char *path){
	struct stat st;
	
	if (stat(path, &st)<0){
		err (1, "path\n");
	}
	if ((st.st_mode & S_IFMT) == S_IFDIR)
		return 1;
	else
		return 0;
}

//Comprobar si es fichero
static int
es_fich (char *path){
	struct stat st;
	
	if (stat(path, &st)<0){
		err (1, "path");
	}
	if((st.st_mode & S_IFMT) == S_IFREG)
		return 1;
	else
		return 0;
}

//Comprobar si una palabra está en el fichero
static int
esta_en_fich (char *palabra, char *fichero) {
	int fd = 0;
	char buff [strlen(palabra)];
	
	fd = open (fichero, O_RDONLY);
	if (fd <0){
		err(1, "%s", fichero);
	}
	read(fd, buff, strlen(palabra));
	close(fd);
	if (!strncmp(buff, palabra, strlen(palabra))){
		return 1;
	}else{
		return 0;
	}
}

//Ver si es . o ..
static int
file_no_acceder (char *file_name){
	int i = 0;
	int long_array_files= sizeof(No_Entrar)/sizeof(*No_Entrar);
	for (i = 0; i<long_array_files; i++){
		if (!strcmp((file_name), No_Entrar[i])){
			return 1;
		}	
	}
	return 0;
}

//Buscar una palabra en los ficheros de un (sub)directorio
static void
buscar_en_dir(char *palab, char *path){
	DIR *dir_act;
	struct dirent *file;
	char path_aux[100];
	
	dir_act = opendir(path);
	if (dir_act == NULL){
		err (1, "%s", path);
	}
	while((file=readdir(dir_act))!=NULL){
		if (file_no_acceder(file->d_name)){
			continue;
		}
		strcpy(path_aux, path);
		strcat (path_aux, "/");
		strcat (path_aux,file->d_name);
		if (es_dir(path_aux)){
			buscar_en_dir (palab, path_aux);
		}else if (es_fich(path_aux)){
			if (esta_en_fich (palab, path_aux)){
				printf("%s\n", path_aux);
			}
		}
	}
	closedir(dir_act);
}

int
main( int argc, char *argv[]){
	if (argc==1){
		fprintf(stderr, "usage: %s pal [directorios]\n", argv[0]);
	}
	argc--;
	argv++;
	char *palab_comp = argv[0];
	argc--;
	argv++;
	int i = 0;		 //Contador
	if (argc==0){
		buscar_en_dir(palab_comp, ".");
	}else{
		for (i = 0; i<argc; i++){
			buscar_en_dir (palab_comp, argv[i]);
		}
	}
	
	exit(0);
}
