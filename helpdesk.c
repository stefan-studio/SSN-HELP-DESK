#include "helpdesk.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

#define TICKET_DB_MAGIC "HELPDSK"
#define TICKET_DB_VERSION 1

Ticket* ticketHead = NULL;
int nextTicketId = 1000;

// Simple hardcoded engineers for demo purposes, as no engineers.db is implemented in C yet
typedef struct { int id; const char* name; const char* specialty; } Engineer;
Engineer engineers[] = {
    {101, "Alice Smith", "Network"}, {102, "Bob Johnson", "Hardware"},
    {103, "Charlie Brown", "Software"}, {104, "Diana Prince", "Plumbing"}
};
const char *issueNames[] = {
    "Furniture", "WiFi", "Network", "Hardware", "Software", "Other",
    "Plumbing", "Electrical", "Security", "ID Card"
};
const char *statusNames[] = {"Open", "Assigned", "In Progress", "Resolved", "Closed"};

void addTicketByPriority(Ticket* newT) {
    if (!ticketHead || newT->priority > ticketHead->priority) {
        newT->next = ticketHead;
        ticketHead = newT;
        return;
    }
    Ticket* temp = ticketHead;
    while (temp->next && temp->next->priority >= newT->priority) {
        temp = temp->next;
    }
    newT->next = temp->next;
    temp->next = newT;
}

void printJsonString(const char *text) {
    putchar('"');
    for (const char *p = text; *p; ++p) {
        switch (*p) {
            case '\\': printf("\\\\"); break;
            case '"': printf("\\\""); break;
            case '\b': printf("\\b"); break;
            case '\f': printf("\\f"); break;
            case '\n': printf("\\n"); break;
            case '\r': printf("\\r"); break;
            case '\t': printf("\\t"); break;
            default:
                if ((unsigned char)*p < 0x20) {
                    printf("\\u%04x", (unsigned char)*p);
                } else {
                    putchar(*p);
                }
        }
    }
    putchar('"');
}

int syncTickets(void) {
    FILE *f = fopen("tickets.db", "wb");
    if (!f) return -1;

    TicketFileHeader header;
    memcpy(header.magic, TICKET_DB_MAGIC, sizeof(header.magic));
    header.version = TICKET_DB_VERSION;
    header.nextTicketId = nextTicketId;
    fwrite(&header, sizeof(header), 1, f);

    for (Ticket* t = ticketHead; t; t = t->next) {
        TicketRecord rec;
        rec.id = t->id;
        rec.uid = t->uid;
        rec.engineerId = t->engineerId;
        rec.priority = t->priority;
        rec.issueType = t->issueType;
        rec.status = t->status;
        rec.timeCreated = t->timeCreated;
        strncpy(rec.description, t->description, MAX_DESCRIPTION_LEN - 1);
        rec.description[MAX_DESCRIPTION_LEN - 1] = '\0';
        fwrite(&rec, sizeof(rec), 1, f);
    }

    fclose(f);
    return 0;
}

void loadTickets(void) {
    FILE *f = fopen("tickets.db", "rb");
    if (!f) return;

    TicketFileHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1 ||
        memcmp(header.magic, TICKET_DB_MAGIC, sizeof(header.magic)) != 0 ||
        header.version != TICKET_DB_VERSION) {
        fclose(f);
        return;
    }
    if (header.nextTicketId > 1000) {
        nextTicketId = header.nextTicketId;
    }

    TicketRecord rec;
    while (fread(&rec, sizeof(rec), 1, f) == 1) {
        Ticket* newT = (Ticket*)malloc(sizeof(Ticket));
        if (!newT) break;
        newT->id = rec.id;
        newT->uid = rec.uid;
        newT->engineerId = rec.engineerId;
        newT->priority = rec.priority;
        newT->issueType = rec.issueType;
        newT->status = rec.status;
        newT->timeCreated = rec.timeCreated;
        strncpy(newT->description, rec.description, MAX_DESCRIPTION_LEN - 1);
        newT->description[MAX_DESCRIPTION_LEN - 1] = '\0';
        newT->next = NULL;
        addTicketByPriority(newT);
    }

    fclose(f);
}

void freeTickets(void) {
    Ticket* curr = ticketHead;
    while (curr) {
        Ticket* next = curr->next;
        free(curr);
        curr = next;
    }
    ticketHead = NULL;
}

int closeTicket(int ticketId) {
    Ticket *curr = ticketHead;
    while (curr) {
        if (curr->id == ticketId) {
            curr->status = CLOSED;
            // Optionally, add a timeClosed field to Ticket struct and set it here
            syncTickets();
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

int updateTicketStatus(int ticketId, const char *newStatusStr) {
    Ticket *curr = ticketHead;
    while (curr) {
        if (curr->id == ticketId) {
            for (int i = 0; i < (int)(sizeof(statusNames) / sizeof(statusNames[0])); i++) {
                if (strcasecmp(statusNames[i], newStatusStr) == 0) {
                    curr->status = (enum TicketStatus)i;
                    syncTickets();
                    return 1;
                }
            }
            return 0;
        }
        curr = curr->next;
    }
    return 0;
}

// Modified to return the updated ticket details
// This helps Flask update its state without re-fetching all tickets
int assignTicket(int ticketId, int engineerId) {
    Ticket *curr = ticketHead;
    while (curr) {
        if (curr->id == ticketId) {
            curr->engineerId = engineerId;
            curr->status = ASSIGNED;
            syncTickets();
            // Return the updated ticket details for Flask
            printf("{\"success\":true,\"ticket\":{\"id\":%d,\"uid\":%d,\"engineer_id\":%d,\"priority\":%d,\"issue_type\":\"%s\",\"status\":\"%s\",\"description\":",
                   curr->id, curr->uid, curr->engineerId, curr->priority,
                   issueNames[curr->issueType], statusNames[curr->status]);
            printJsonString(curr->description);
            char timeBuf[20];
            struct tm *tm_info = localtime(&curr->timeCreated);
            strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%S", tm_info);
            printf(",\"created_at\":\"%s\"}}\n", timeBuf);
            return 1; // Indicate success
        }
        curr = curr->next;
    }
    return 0;
}

int deleteTicket(int ticketId) {
    Ticket *current = ticketHead;
    Ticket *prev = NULL;

    while (current && current->id != ticketId) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) { // Ticket not found
        return 0; // Failure
    }

    if (prev == NULL) { // Deleting the head node
        ticketHead = current->next;
    } else { // Deleting a middle or tail node
        prev->next = current->next;
    }

    free(current);
    syncTickets(); // Persist changes
    return 1; // Success
}

int editTicket(int ticketId, const char *newIssueTypeStr, const char *newDescription, int newPriority, const char *newStatusStr) {
    Ticket *current = ticketHead;
    Ticket *prev = NULL;

    // Find the ticket to edit
    while (current && current->id != ticketId) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) { // Ticket not found
        return 0; // Failure
    }

    // Store old priority to check if re-insertion is needed
    int oldPriority = current->priority;

    // Update fields
    strncpy(current->description, newDescription, MAX_DESCRIPTION_LEN - 1);
    current->description[MAX_DESCRIPTION_LEN - 1] = '\0';
    current->priority = newPriority;

    // Convert issue type string to enum
    for (int i = 0; i < sizeof(issueNames) / sizeof(issueNames[0]); i++) {
        if (strcasecmp(issueNames[i], newIssueTypeStr) == 0) {
            current->issueType = (enum IssueType)i;
            break;
        }
    }

    // Convert status string to enum
    for (int i = 0; i < sizeof(statusNames) / sizeof(statusNames[0]); i++) {
        if (strcasecmp(statusNames[i], newStatusStr) == 0) {
            current->status = (enum TicketStatus)i;
            break;
        }
    }

    // If priority changed, we need to re-insert the ticket to maintain sorted order
    if (current->priority != oldPriority) {
        // Remove from current position
        if (prev == NULL) { // It was the head
            ticketHead = current->next;
        } else {
            prev->next = current->next;
        }
        current->next = NULL; // Detach from old list

        // Re-add to maintain priority order
        addTicketByPriority(current);
    }

    syncTickets(); // Persist changes
    return 1; // Success
}

Ticket *createTicket(int uid, const char *issueType, const char *description, int priority) {
    Ticket* t = (Ticket*)malloc(sizeof(Ticket));
    if (!t) return NULL;

    // Ensure priority is within bounds
    if (priority < 1) priority = 1;
    if (priority > 5) priority = 5;

    // Initialize new ticket fields
    t->id = nextTicketId++;
    t->uid = uid;
    t->engineerId = 0; // Unassigned
    t->priority = priority;
    t->status = OPEN;
    t->issueType = OTHER;

    // Map issueType string to enum
    for(int i=0; i < (int)(sizeof(issueNames)/sizeof(issueNames[0])); i++) {
        if(strcasecmp(issueNames[i], issueType) == 0) { t->issueType = i; break; }
    }
    strncpy(t->description, description, MAX_DESCRIPTION_LEN - 1);
    t->description[MAX_DESCRIPTION_LEN-1] = '\0';
    t->timeCreated = time(NULL);
    t->next = NULL;

    addTicketByPriority(t);
    syncTickets();

    // Print the full ticket object for Flask
    printf("{\"success\":true,\"ticket\":{\"id\":%d,\"uid\":%d,\"engineer_id\":%d,\"priority\":%d,\"issue_type\":\"%s\",\"status\":\"%s\",\"description\":",
           t->id, t->uid, t->engineerId, t->priority,
           issueNames[t->issueType], statusNames[t->status]);
    printJsonString(t->description);
    char timeBuf[20];
    struct tm *tm_info = localtime(&t->timeCreated);
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%S", tm_info);
    printf(",\"created_at\":\"%s\"}}\n", timeBuf);

    return t;
}

void listEngineers(void) {
    printf("{\"success\":true,\"engineers\":[");
    for (size_t i = 0; i < sizeof(engineers) / sizeof(engineers[0]); i++) {
        if (i > 0) printf(",");
        printf("{\"id\":%d,\"name\":\"%s\",\"specialty\":\"%s\",\"active_tickets\":0,\"total_resolved\":0}",
               engineers[i].id, engineers[i].name, engineers[i].specialty);
    }
    printf("]}\n");
}


int main(int argc, char *argv[]) {
    srand(time(NULL));
    loadTickets();
    if (argc < 2) return 0;

    if (strcmp(argv[1], "login") == 0 && argc >= 4) {
        // Simple auth logic for demo
        if(strcmp(argv[2], "admin") == 0 && strcmp(argv[3], "admin123") == 0)
            printf("{\"success\":true,\"user_id\":1,\"role\":\"admin\"}\n");
        else if(strcmp(argv[2], "engineer") == 0 && strcmp(argv[3], "eng123") == 0)
            printf("{\"success\":true,\"user_id\":201,\"role\":\"engineer\"}\n");
        else if(strcmp(argv[3], "1234") == 0)
            printf("{\"success\":true,\"user_id\":101,\"role\":\"user\"}\n");
        else
            printf("{\"success\":false,\"error\":\"Invalid Credentials\"}\n");
    } 
    else if (strcmp(argv[1], "create_ticket") == 0 && argc >= 6) {
        if (createTicket(atoi(argv[2]), argv[3], argv[4], atoi(argv[5]))) {
        } else {
            printf("{\"success\":false,\"error\":\"Memory allocation failed\"}\n");
        }
    } 
    else if (strcmp(argv[1], "list_tickets") == 0) {
        int filterUid = (argc >= 3) ? atoi(argv[2]) : 0;
        int filterEng = (argc >= 4) ? atoi(argv[3]) : 0;

        printf("{\"success\":true,\"tickets\":[");
        int first = 1;
        for (Ticket* t = ticketHead; t; t = t->next) {
            if ((filterUid == 0 || t->uid == filterUid) && (filterEng == 0 || t->engineerId == filterEng)) {
                if (!first) printf(",");
                printf("{\"id\":%d,\"status\":\"%s\",\"issue_type\":\"%s\",\"engineer_id\":%d,\"description\":", 
                       t->id, statusNames[t->status], issueNames[t->issueType], t->engineerId);
                printJsonString(t->description);
                
                char timeBuf[20];
                struct tm *tm_info = localtime(&t->timeCreated);
                strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%S", tm_info);
                printf(",\"priority\":%d,\"created_at\":\"%s\"}", t->priority, timeBuf);
                first = 0;
            }
        }
        printf("]}\n");
    }
    else if (strcmp(argv[1], "delete_ticket") == 0 && argc >= 3) {
        int ticketId = atoi(argv[2]);
        if (deleteTicket(ticketId)) {
            printf("{\"success\":true,\"message\":\"Ticket %d deleted\"}\n", ticketId);
        } else {
            printf("{\"success\":false,\"error\":\"Ticket %d not found\"}\n", ticketId);
        }
    }
    else if (strcmp(argv[1], "edit_ticket") == 0 && argc >= 7) {
        int ticketId = atoi(argv[2]);
        const char *newIssueType = argv[3];
        const char *newDescription = argv[4];
        int newPriority = atoi(argv[5]);
        const char *newStatus = argv[6];
        if (editTicket(ticketId, newIssueType, newDescription, newPriority, newStatus)) {
            printf("{\"success\":true,\"message\":\"Ticket %d updated\"}\n", ticketId);
        } else {
            printf("{\"success\":false,\"error\":\"Ticket %d not found or update failed\"}\n", ticketId);
        }
    }
    else if (strcmp(argv[1], "assign_ticket") == 0 && argc >= 4) {
        int tid = atoi(argv[2]);
        int eid = atoi(argv[3]);
        if (!assignTicket(tid, eid)) { // assignTicket now prints its own JSON
            printf("{\"success\":false,\"error\":\"Ticket not found\"}\n");
        }
    }
    else if (strcmp(argv[1], "close_ticket") == 0 && argc >= 3) {
        int ticketId = atoi(argv[2]);
        if (closeTicket(ticketId)) {
            printf("{\"success\":true,\"message\":\"Ticket %d closed\"}\n", ticketId);
        } else {
            printf("{\"success\":false,\"error\":\"Ticket %d not found\"}\n", ticketId);
        }
    }
    else if (strcmp(argv[1], "list_engineers") == 0) {
        listEngineers();
    }
    else if (strcmp(argv[1], "update_status") == 0 && argc >= 4) {
        int ticketId = atoi(argv[2]);
        const char *newStatus = argv[3];
        if (updateTicketStatus(ticketId, newStatus))
            printf("{\"success\":true,\"message\":\"Status updated\"}\n");
        else
            printf("{\"success\":false,\"error\":\"Update failed\"}\n");
    }
    freeTickets();
    return 0;
}
