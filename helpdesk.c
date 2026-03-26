#include <stdio.h>
#include <time.h>
#include <string.h>
enum IssueType {
    FURNITURE = 0,
    WIFI = 1,
    NETWORK = 2,
    HARDWARE = 3,
    SOFTWARE = 4,
    OTHER = 5
};


// Helper array to convert enum to readable strings
const char* issueNames[] = {
    "Furniture",
    "WiFi",
    "Network",
    "Hardware",
    "Software",
    "Other"
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

typedef struct
{
    char issueDescription[256];
    char closingNotes[256];
    time_t timeCreated;
    time_t timeAssigned;
    time_t timeClosed;
    enum IssueType issueType;
    int id;
    int uid;
    int eid;
    int status;
}Ticket;

//Loads existing data from the database into the Server
int init()
{

}

//Save tickets in server to the database
int syncTickets()
{

}

//authenticate user info with db
int authenticateUser(char* username,char* password)
{

}

//returns a unique ticket id
int genTicketID()
{

}
//Create and return new tickets
Ticket* createTicket(int uid,char* issue)
{

}

//Closes an open ticket after work is done
void closeTicket(Ticket* ticket)
{

}

//assigns a service engineer to the issue
void assignEngineer(Ticket* ticket)
{

}

int main()
{

}