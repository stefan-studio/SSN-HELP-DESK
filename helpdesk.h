#ifndef HELPDESK_H
#define HELPDESK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_DESCRIPTION_LEN  256
#define MAX_USERNAME_LEN     50

enum IssueType { 
    FURNITURE, WIFI, NETWORK, HARDWARE, SOFTWARE, OTHER, 
    PLUMBING, ELECTRICAL, SECURITY, ID_CARD 
};
enum TicketStatus { OPEN, ASSIGNED, IN_PROGRESS, RESOLVED, CLOSED };

typedef struct {
    char magic[8];
    int version;
    int nextTicketId;
} TicketFileHeader;

typedef struct {
    int id;
    int uid;
    int engineerId;
    int priority;
    enum IssueType issueType;
    enum TicketStatus status;
    char description[MAX_DESCRIPTION_LEN];
    time_t timeCreated;
} TicketRecord;

typedef struct Ticket {
    int id;
    int uid;
    int engineerId;
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
int closeTicket(int ticketId);
int updateTicketStatus(int ticketId, const char *newStatusStr);
int assignTicket(int ticketId, int engineerId);
int deleteTicket(int ticketId);
int editTicket(int ticketId, const char *newIssueTypeStr, const char *newDescription, int newPriority, const char *newStatusStr);
void listEngineers(void); // New function prototype
void freeTickets(void);
Ticket* createTicket(int uid, const char *issueType, const char *description, int priority);

#endif