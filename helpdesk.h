#ifndef HELPDESK_H
#define HELPDESK_H

#include <time.h>

typedef struct Ticket {
    int id;
    int uid;
    char block[30];      // Added
    char room[20];       // Added
    char category[50];   // Expanded
    char phone[15];      // Added
    char description[512];
    char evidencePath[256];
    char status[20];
    struct Ticket* next;
} Ticket;

// Syllabus: Struct for the Queue
typedef struct {
    Ticket* front; // Points to the first ticket in line
    Ticket* rear;  // Points to the last ticket in line
} TicketQueue;

// Function Prototypes for Module A
void initQueue(TicketQueue* q);
void enqueue(TicketQueue* q, int uid, char* cat, char* desc, char* file);
Ticket* dequeue(TicketQueue* q);
int genTicketID();

#endif

