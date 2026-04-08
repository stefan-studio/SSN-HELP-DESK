#ifndef HELPDESK_H
#define HELPDESK_H

#include <time.h>

// Syllabus: Struct for a Linked List Node
typedef struct Ticket {
    int id;                 // Ticket ID
    int uid;                // Student/User ID
    char category[30];      // SCRUM-4: Hostel, Dept, General
    char description[256];  // The issue details
    char evidencePath[256]; // SCRUM-5: Path to the uploaded photo
    char status[20];        // Open, Assigned, Closed
    time_t timeCreated;
    struct Ticket* next;    // Pointer to the next node (Linked List)
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

