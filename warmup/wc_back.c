#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "wc.h"

struct wc {
	/* you can define this struct to have whatever fields you want. */
	struct node *head[10000];
};

struct node {
	int count;	//value
	char word[300];	//key
	struct node *next;
};

/*Credits to Daniel Berstein's String Hasing Function*/

unsigned long djb2hash(char *str){
	unsigned long hash_index = 5381;
	int c=0;
	
	while((c = *str++)){
		hash_index = ((hash_index << 5) + hash_index) + c;
	}
	return hash_index;
	
}		


struct wc *
wc_init(char *word_array, long size)
{
	struct wc *wc;
	
	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);
	int j = 0;
	while(j<10000){
		wc->head[j] = NULL;
		j++;
	}
	/* Seperate characters to words*/
	off_t i = 0;
	char character;
	char word[300]; //300 buffer
	int length = 0;
	//printf ("%zu", sb.st_size);
	while(i < size){
		character = ((char *)word_array)[i];

		/*if (character == '(' || character == ')' || character == ',' || character == ';' || character == '.' || character == ':' || character == '?' || character == '!'){
			i++;			
			continue;
		}*/		
				
		if (isspace(character)) {

			if (length > 0){
				word[length] = 0; //end the word with null character
				length = 0;
				add_word(wc , word);
			}
		}
		else{
			//character = tolower(character);			
			word[length] = character;
			length++;
		}
		i++;
	}
	
	return wc;
}

int add_word(struct wc* wc, char *word){
	
	unsigned long index = djb2hash(word)%10000;
	struct node *pointer = wc->head[index]; //Pointer to this hash location
	
	if (pointer == NULL){
		struct node *new_word = (struct node*)malloc(sizeof(struct node));	//new word node	
		strcpy(new_word->word , word);
		new_word->count = 1;
		new_word->next = NULL;
		wc->head[index] = new_word;		//if pointer is empty slot in hash, store new word.
		return 1;
	}
	else {
		while(pointer->next != NULL){	//traverse through the hash
			if(strcmp(pointer->word,word) ==0){
				pointer->count++;
				return 1;
			}
			pointer = pointer->next;
		}
		if (strcmp(pointer->word,word)==0){
			pointer->count++;
			return 1;
		}
		else if(pointer->next == NULL){
			struct node *new_word = (struct node*)malloc(sizeof(struct node));	//new word node			
			strcpy(new_word->word , word);
			new_word->count = 1;
			new_word->next = NULL;			
			pointer->next = new_word;	//add new word at the next
			return 1;
		}
	}
	return 0;
}

void
wc_output(struct wc *wc)
{
	int i=0;
	
	struct node * pointer;
	
	for (i=0; i < 10000; i++){
		pointer = wc->head[i];
		while(pointer!=NULL){
			printf("%s:%d\n",pointer->word, pointer->count);
			pointer = pointer->next;
		}
		
	}
}

void
wc_destroy(struct wc *wc)
{
	struct node * temp;
	
	int i =0;
	
	if (wc->head == NULL){
		free(wc);		
		return;	
	}	
	for (i=0; i < 10000; i++){
		struct node *pointer = wc->head[i];
		while(pointer!=NULL){
			temp = pointer->next;		
			free(pointer);
			pointer = temp;							
		}
		
	}
	free(wc);
}
