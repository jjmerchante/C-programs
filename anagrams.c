/**
* anagrams.c
* Copyright Jose J. Merchante 
* jjmerchante@gmail.com
* 2014
* Guiven a list of words as argument, returns
* which are anagrams and letters that are in the
* same position
*
* Compile: ggc -o anagrams anagrams.c
* Run: ./anagrams mora roma ...
**/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum {
	Num_Min_Args = 3
};

struct lista_pals {
	char *pal_list;
	struct lista_pals *next;
};
typedef struct lista_pals lista_pals;

static int
veces_letra (char letra, char *pal) {
	char letra_de_pal = *pal;
	int k = 0;		/*Para indicar posicion*/
	int veces = 0;
	while (letra_de_pal != '\0'){
		if (letra_de_pal == letra){
			veces = veces + 1;
		}
		k= k + 1;
		letra_de_pal = *(pal + k);
	}
	return veces;
}

static int
son_anagrams (char *pal_1, char *pal_2){
	int veces_en_1;
	int veces_en_2;
	int k = 0;			/*Para indicar posicion*/
	char letra = pal_1[k];
	if (strlen(pal_1) != strlen(pal_2)){
		return 0;
	}
	while (letra != '\0'){
		veces_en_1 = veces_letra (letra, pal_1);
		veces_en_2 = veces_letra (letra, pal_2);
		if (veces_en_1 == veces_en_2) {
			k++;
			letra = pal_1[k];
		} else {
			return 0;
		}
	}
	return 1;
}

static void
print_pal_anagrs(lista_pals *lista_arg){
	lista_pals *p_aux= lista_arg;
	while (p_aux !=NULL){
		printf("%s ", p_aux->pal_list);
		p_aux=p_aux->next;
	}
}

static lista_pals
*add_pal(lista_pals *list, char *palabra){
	if (list == NULL){
		list = malloc(sizeof(lista_pals));
		list->next = NULL;
		list->pal_list= strdup(palabra);
	}else{
		lista_pals *p_aux=list;
		list = malloc(sizeof(lista_pals));
		list->pal_list= strdup(palabra);
		list->next = p_aux;
	}
	return list;
}

static lista_pals
*get_pal_anagrs (lista_pals *lista, char *main_word){
	lista_pals *p_aux = lista->next;
	lista_pals *args_list = NULL;
	while (p_aux != NULL){
		if (son_anagrams(main_word, p_aux->pal_list)){
			args_list = add_pal(args_list, p_aux->pal_list);
		}
		p_aux = p_aux->next;
	}
	if (args_list != NULL){
		args_list = add_pal(args_list, main_word);
	}
	return args_list;
}

static lista_pals
*rm_pal_anagrs (lista_pals *lista, char *main_word){
	 lista_pals *p_aux =lista;
	 lista_pals *p_prev = NULL;
	while (p_aux != NULL){
		if (son_anagrams(main_word, p_aux->pal_list)){
			free(p_aux->pal_list);
			if (p_prev==NULL){
				lista = p_aux->next;
				free(p_aux);
				p_aux = lista;
			}else{
				p_aux = p_aux->next;
				free(p_prev->next);
				p_prev->next = p_aux;
			}
		} else {
			p_prev = p_aux;
			p_aux = p_aux->next;
		}
	}
	return lista;
}

static void
print_mism_letra(lista_pals *lista){
	lista_pals *p_aux =lista->next;
	char *pal_main = lista->pal_list;
	char *pal_comp;
	int iguales;
	int k = 0;		/*Contador*/
	int len = strlen (pal_main);
	printf("[");
	for (k=0; k<len;k++){
		p_aux=lista->next;
		iguales = 1;
		while (iguales && (p_aux != NULL)){
			pal_comp = p_aux->pal_list;
			if (pal_main[k]!=pal_comp[k])
			{
				iguales = 0;
			}
			p_aux=p_aux->next;
		}
		if (iguales){
			printf("%c",pal_main[k]);
		}
	}
	printf("]\n");
}

static void
buscar_anagr(lista_pals *lista){
	lista_pals *p_aux = lista;
	lista_pals *pal_anagrams = NULL; 
	while (p_aux !=NULL){
		pal_anagrams = get_pal_anagrs(lista, p_aux->pal_list);
		if (pal_anagrams != NULL){
			print_pal_anagrs(pal_anagrams);
			print_mism_letra (pal_anagrams);
		}
		lista = rm_pal_anagrs (lista, strdup(p_aux->pal_list));
		p_aux = lista;
	}
}

int
main (int argc, char *argv [])
{
	if (argc <Num_Min_Args) {
		printf("¡Escribe al menos 2 palabras!\n");
		exit(1);
	}
	
	lista_pals *list_argums = NULL;
	int i = 0;
	for (i=1; i<argc;i++){
		list_argums = add_pal(list_argums, argv[i]);
	}
	buscar_anagr(list_argums);
	exit(0);
}