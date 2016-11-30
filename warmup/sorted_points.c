#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "common.h"
#include "point.h"
#include "sorted_points.h"

struct sorted_points { //Wrapper for linked list
	/* you can define this struct to have whatever fields you want. */
	struct node *head;
};

struct node {
	struct point *p1;
	struct node *next;
};

struct sorted_points *
sp_init()
{
	struct sorted_points *sp;
	
	sp = (struct sorted_points *)malloc(sizeof(struct sorted_points));
	
	assert(sp);
	sp->head =NULL; 
	//TBD();
	
	return sp;
}

void
sp_destroy(struct sorted_points *sp)
{
	struct node *current = sp->head;
	struct node *next;
	while(current != NULL){
		next = current->next;
		free (current->p1);
		free(current);
		current = next;
	}
	
	//TBD();
	free(sp);
}

int
sp_add_point(struct sorted_points *sp, double x, double y) //sp points to the head
{
	struct node *temp = (struct node*)malloc(sizeof(struct node));
	struct point *point = (struct point*)malloc(sizeof(struct point));
	struct node *current = sp->head;
	struct node *previous = NULL;
	point->x = x;
	point->y = y;		
	temp->p1 = point;
	//free (point);
	
	if (sp->head == NULL){	//new list
		sp->head = temp;
		temp->next = NULL;
		return 1;
	}
	if (point_compare(sp->head->p1, temp->p1) == 1){	//new head
		temp->next= sp->head;
		sp->head = temp;
		return 1;
	}
	
	if (point_compare(sp->head->p1, temp->p1) == 0){	//same distance but temp has smaller or equal x value
		if (sp->head->p1->x >= temp->p1->x){
			temp->next = sp->head;
			sp->head = temp;
			return 1;			
		}
	}
		
	while(current!=NULL){
		int calc_distance = point_compare(current->p1,temp->p1);
		if ((calc_distance == 0 && current->p1->x > temp->p1->x) || calc_distance == 1){ // if equals 1, then temp<current
			previous ->next = temp;
			temp->next = current;
			return 1;					
		}
		previous = current;
		current = current->next;
	}
	if (point_compare(previous->p1,temp->p1) == -1 || (point_compare(previous->p1,temp->p1) == 0 && previous->p1->x <= temp->p1->x)){
		previous->next = temp; 	
		temp->next=NULL;
		return 1;
	}
	
	return 0;
}

int
sp_remove_first(struct sorted_points *sp, struct point *ret)
{
	struct node *temp = sp->head;
	if(temp != NULL){
		sp-> head= temp->next;
		ret->x = temp->p1->x;
		ret->y = temp->p1->y;
		free(temp->p1);		
		free(temp);
		return 1;
	}
	//TBD();
	return 0;
}

int
sp_remove_last(struct sorted_points *sp, struct point *ret)
{
	struct node *temp = sp->head;
	struct node *previous = NULL;
	if(sp->head == NULL){
		return 0; //empty list
	}
	else if (temp->next == NULL){ //one node
		sp->head=NULL;
		ret->x = temp->p1->x;
		ret->y = temp->p1->y;
		free(temp->p1);
		free(temp);
		return 1;
	}
	else{
		while(temp->next!=NULL){
			previous = temp;
			temp = temp -> next;
		}
		previous->next=NULL;
		ret -> x = temp->p1->x;
		ret -> y = temp->p1->y;
		free(temp->p1);
		free(temp);
		return 1;
	}
	
	//TBD();
	return 0;
}

int
sp_remove_by_index(struct sorted_points *sp, int index, struct point *ret)
{
	int count = 0;
	struct node *current = sp->head;
	if (current == NULL || index < 0){
		return 0;
	}
	if (index == 0){ //Remove Head
		return sp_remove_first(sp,ret);
		free(current);
	}
	else {
		//struct sorted_points *prev = NULL;
		while(index-1 > count){
			if (current == NULL){
				return 0; 		//index out of bounds
			}
			current = current->next;
			
			count++;
		}
		struct node *temp2 =current->next; //node to be deleted
	
		if (temp2 == NULL){
			return 0;		//index out of bounds		
		}		
				
		current->next = temp2 -> next;
		ret->x = temp2->p1->x;
		ret->y = temp2->p1->y;
		free(temp2->p1);		
		free(temp2);
		return 1;
		}
	//TBD();
	return 0;
}

int
sp_delete_duplicates(struct sorted_points *sp)
{
	int delete_count = 0;
	struct node *current = sp->head;
	struct node *temp;
	if (current == NULL){	//if 0 or 1 node in linked list
		return delete_count;
	}
	
	while(current->next != NULL){
		if ((current->p1->x == current->next->p1->x) && (current->p1->y == current->next->p1->y)){
			temp = current->next->next;
			free(current->next->p1);			
			free(current->next);
			current->next = temp;
			delete_count++;
		}
		else{
			current = current -> next;
		}
	}
	//TBD();
	return delete_count;
}
/*void
print_list(struct sorted_points *sp){
	struct node* temp = sp->head;
	while(temp!=NULL){
		printf("x:%f and y:%f\n", temp->p1->x, temp->p1->y);
		temp = temp->next;
	}
}*/
