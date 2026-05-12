#ifndef HELPDESK_H
#define HELPDESK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_USERS            500
#define MAX_ENGINEERS        100
#define MAX_TICKETS_HEAP     10000 // Max capacity for our Min-Heap
#define MAX_USERNAME_LEN     50
#define MAX_PASSWORD_LEN     50
#define MAX_DESCRIPTION_LEN  256
#define MAX_NOTES_LEN        512

enum IssueType { FURNITURE = 0, WIFI = 1, NETWORK = 2, HARDWARE = 3, SOFTWARE = 4, OTHER = 5 };
enum TicketStatus { OPEN = 0, ASSIGNED = 1, IN_PROGRESS = 2, RESOLVED = 3, CLOSED = 4 };
enum UserRole { STAFF = 0, ENGINEER = 1, MANAGER = 2, MIDDLEMAN = 3 }; // Added MIDDLEMAN

typedef struct {
    int           id;
    char          username[MAX_USERNAME_LEN];
    char          password[MAX_PASSWORD_LEN];
    enum UserRole role;
    char          name[MAX_USERNAME_LEN];
} User;

typedef struct {
    int            id;
    char           name[MAX_USERNAME_LEN];
    enum IssueType specialty;
    int            ticketsAssigned;
    int            ticketsResolved;
} Engineer;

typedef struct {
    time_t         timeCreated;
    time_t         timeAssigned;
    time_t         timeClosed;
    enum IssueType issueType;
    char           description[MAX_DESCRIPTION_LEN];
    int            id;
    int            uid;
    int            eid;
    int            priority; // 1 (Highest) to 5 (Lowest). 0 = Unassigned
    enum TicketStatus status;
    char           notes[MAX_NOTES_LEN];
    char           imagePath[256];
} Ticket;

typedef struct TicketNode {
    Ticket data;
    struct TicketNode* leftChild;
    struct TicketNode* rightChild;
} TicketNode;

#endif /* HELPDESK_H */