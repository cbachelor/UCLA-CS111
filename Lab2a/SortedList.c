#include "SortedList.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    if(element == NULL) {
        fprintf(stderr, "Error: Cannot insert an element of NULL\n");
        exit(1);
    }
    if(list->key != NULL) {
        fprintf(stderr, "Error: Please pass the head for the list you want to insert\n");
        exit(1);
    }
    SortedListElement_t* temp = list->next; 
    while(temp != list) {
        if(strcmp(temp->key,element->key) >= 0){
            break;
        }
        temp = temp->next;
    }
    if(opt_yield & INSERT_YIELD){
                sched_yield();
            }
    element->next = temp;
    element->prev = temp->prev;
    temp->prev->next = element;
    temp->prev = element;
    
}

int SortedList_delete( SortedListElement_t *element){
    if (element->next->prev == element && element->prev->next == element) {
        if(opt_yield & DELETE_YIELD) {
            sched_yield();
        }
        element->next->prev = element->prev;
        element->prev->next = element->next;
        element->prev = NULL;
        element->next = NULL;
        return 0;
    }
    else {
        return 1;
    }
}


SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    if(list->key != NULL) {
        fprintf(stderr, "Error: Please pass the head for the list you want to insert\n");
        exit(1);
    }
    SortedListElement_t* temp = list->next;
    while(temp != list) {
        if(strcmp(temp->key, key) == 0)  {
            if(opt_yield & LOOKUP_YIELD) {
                sched_yield();
            }
            return temp;
        } 
        temp = temp->next;
    }
    return NULL;
}

int SortedList_length(SortedList_t *list) {
     int length = 0;
     SortedList_t* temp = list->next;
     while(temp != list) {
        if (temp->next == list) {
            if(opt_yield & LOOKUP_YIELD) {
                sched_yield();
            }
            break;
        }
        else if(temp->next->prev == temp && temp->prev->next == temp) {
            if(opt_yield & LOOKUP_YIELD) {
                sched_yield();
            }
            length++;
            temp = temp->next;
        }
        else
            return -1;
 
     }
     return length;
}
