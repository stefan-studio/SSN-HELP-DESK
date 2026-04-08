#include "helpdesk.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_TICKETS 1000
#define MAX_USERS 500
#define MAX_ENGINEERS 100
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50
#define MAX_DESCRIPTION_LEN 256

static Ticket   tickets[MAX_TICKETS];
static User     users[MAX_USERS];
static Engineer engineers[MAX_ENGINEERS];
static int ticketCount   = 0;
static int userCount     = 0;
static int engineerCount = 0;

const char* issueNames[] = {
    "Furniture",
    "WiFi",
    "Network",
    "Hardware",
    "Software",
    "Other"
};

const char* statusNames[] = {
    "Open",
    "Assigned",
    "In Progress",
    "Resolved",
    "Closed"
};


const char* getIssueName(enum IssueType issue) {
    return issueNames[issue];
}
enum IssueType parseIssueType(const char* issueStr) {
    for (int i = 0; i < (sizeof(issueNames)/sizeof(char*)); i++) {
        if (strcmp(issueNames[i], issueStr) == 0) {
            return (enum IssueType)i;
        }
    }
    return OTHER;
}


// Load tickets from file into memory
static void loadTickets() {
    FILE* f = fopen("tickets.db", "rb");
    if (!f) return;
    fread(&ticketCount, sizeof(int), 1, f);
    fread(tickets, sizeof(Ticket), ticketCount, f);
    fclose(f);
}

//Save tickets in server to the database
int syncTickets()
{
    FILE* f = fopen("tickets.db", "wb");
    if (!f) return -1;
    fwrite(&ticketCount, sizeof(int), 1, f);
    fwrite(tickets, sizeof(Ticket), ticketCount, f);
    fclose(f);
    return 0;
}

//authenticate user info with db
// Returns user id on success, -1 on failure
int authenticateUser(char* username, char* password) {
    for (int i = 0; i < userCount; i++)
        if (strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, password) == 0)
            return users[i].id;
    return -1;
}

//returns a unique ticket id
int genTicketID()
{
    int lastID = 0;
    FILE* f = fopen("ticket_counter.dat", "rb");
    if (f) { fread(&lastID, sizeof(int), 1, f); fclose(f); }
    lastID++;
    f = fopen("ticket_counter.dat", "wb");
    if (f) { fwrite(&lastID, sizeof(int), 1, f); fclose(f); }
    return lastID;
}

void assignEngineer(Ticket* ticket);

//Create and return new tickets
Ticket* createTicket(int uid,char* issue)
{
    if (ticketCount >= MAX_TICKETS) return NULL;

    Ticket* t = &tickets[ticketCount++];
    t->id          = genTicketID();
    t->uid         = uid;
    t->eid         = -1;
    t->issueType   = parseIssueType(issue);
    t->status      = OPEN;
    t->timeCreated = time(NULL);
    t->timeAssigned = 0;
    t->timeClosed  = 0;
    strncpy(t->description, issue, MAX_DESCRIPTION_LEN - 1);
    t->notes[0]    = '\0';

    assignEngineer(t);
    syncTickets();
    return t;
}

//Closes an open ticket after work is done
void closeTicket(Ticket* ticket)
{
    ticket->status    = CLOSED;
    ticket->timeClosed = time(NULL);

    // Update engineer stats
    for (int i = 0; i < engineerCount; i++) {
        if (engineers[i].id == ticket->eid) {
            engineers[i].ticketsResolved++;
            break;
        }
    }
    syncTickets();
    printf("Ticket #%d closed.\n", ticket->id);
}

//assigns a service engineer to the issue
void assignEngineer(Ticket* ticket)
{
    int bestIdx = -1, minLoad = INT_MAX;
    for (int i = 0; i < engineerCount; i++) {
        if (engineers[i].specialty == ticket->issueType &&
            engineers[i].ticketsAssigned < minLoad) {
            minLoad = engineers[i].ticketsAssigned;
            bestIdx = i;
            }
    }
    if (bestIdx == -1) {
        for (int i = 0; i < engineerCount; i++) {
            if (engineers[i].ticketsAssigned < minLoad) {
                minLoad = engineers[i].ticketsAssigned;
                bestIdx = i;
            }
        }
    }
    if (bestIdx == -1) return;

    ticket->eid         = engineers[bestIdx].id;
    ticket->status      = ASSIGNED;
    ticket->timeAssigned = time(NULL);
    engineers[bestIdx].ticketsAssigned++;
}

static void reportAllTickets() {

}

static void reportEngineerLoad()
{

}


int main()
{  
    int choice;
    char issueInput[MAX_DESCRIPTION_LEN];
    int demoUserID = 2510161; // Your ID for the demo

    while (1) {
        printf("\n==============================");
        printf("\n   SSN IT HELPDESK PORTAL   ");
        printf("\n==============================");
        printf("\n1. Raise a New Ticket");
        printf("\n2. Exit System");
        printf("\nChoose an option: ");
        
        if (scanf("%d", &choice) != 1) break;
        getchar(); // This clears the "Enter" key from memory

        if (choice == 1) {
            printf("\nEnter Issue (WiFi/Hardware/Software): ");
            fgets(issueInput, MAX_DESCRIPTION_LEN, stdin);
            issueInput[strcspn(issueInput, "\n")] = 0; // Cleans the text

            // This calls your teammate's core logic!
            Ticket* t = createTicket(demoUserID, issueInput);
            
            if (t != NULL) {
                printf("\n✅ TICKET CREATED!");
                printf("\nID: %d | Assigned to Engineer ID: %d\n", t->id, t->eid);
            }
        } 
        else if (choice == 2) {
            printf("\nExiting... See you tomorrow!\n");
            break;
        }
    }
    return 0;
}

    
