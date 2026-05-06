#ifndef HELPDESK_H
#define HELPDESK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_DESCRIPTION_LEN  256
#define MAX_USERNAME_LEN     50

enum IssueType { FURNITURE, WIFI, NETWORK, HARDWARE, SOFTWARE, OTHER };
enum TicketStatus { OPEN, ASSIGNED, IN_PROGRESS, RESOLVED, CLOSED };

typedef struct {
    char magic[8];
    int version;
    int nextTicketId;
} TicketFileHeader;

typedef struct {
    int id;
    int uid;
    int priority;
    enum IssueType issueType;
    enum TicketStatus status;
    char description[MAX_DESCRIPTION_LEN];
    time_t timeCreated;
} TicketRecord;

typedef struct Ticket {
    int id;
    int uid;
    int priority; // 1-5
    enum IssueType issueType;
    enum TicketStatus status;
    char description[MAX_DESCRIPTION_LEN];
    time_t timeCreated;
    struct Ticket* next; 
} Ticket;

void loadTickets(void);
int syncTickets(void);
void addTicketByPriority(Ticket* newT);
Ticket* createTicket(int uid, const char *issueType, const char *description, int priority);

#endif