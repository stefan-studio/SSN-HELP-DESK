#include "helpdesk.h"

#define TICKET_DB_MAGIC "HELPDSK"
#define TICKET_DB_VERSION 1

Ticket* ticketHead = NULL;
int nextTicketId = 1000;
const char *issueNames[] = {"Furniture", "WiFi", "Network", "Hardware", "Software", "Other"};
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

Ticket *createTicket(int uid, const char *issueType, const char *description, int priority) {
    Ticket* t = (Ticket*)malloc(sizeof(Ticket));
    if (!t) return NULL;
    if (priority < 1) priority = 1;
    if (priority > 5) priority = 5;
    t->id = nextTicketId++;
    t->uid = uid;
    t->priority = priority;
    t->status = OPEN;
    t->issueType = OTHER;
    for(int i=0; i<6; i++) {
        if(strcasecmp(issueNames[i], issueType) == 0) { t->issueType = i; break; }
    }
    strncpy(t->description, description, MAX_DESCRIPTION_LEN - 1);
    t->description[MAX_DESCRIPTION_LEN-1] = '\0';
    t->timeCreated = time(NULL);
    t->next = NULL;
    addTicketByPriority(t);
    syncTickets();
    return t;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    loadTickets();
    if (argc < 2) return 0;

    if (strcmp(argv[1], "login") == 0 && argc >= 4) {
        // Simple auth logic for demo
        if(strcmp(argv[2], "admin") == 0 && strcmp(argv[3], "admin123") == 0)
            printf("{\"success\":true,\"user_id\":1,\"role\":\"admin\"}\n");
        else if(strcmp(argv[3], "1234") == 0)
            printf("{\"success\":true,\"user_id\":101,\"role\":\"user\"}\n");
        else
            printf("{\"success\":false,\"error\":\"Invalid Credentials\"}\n");
    } 
    else if (strcmp(argv[1], "create_ticket") == 0 && argc >= 6) {
        Ticket* t = createTicket(atoi(argv[2]), argv[3], argv[4], atoi(argv[5]));
        printf("{\"success\":true,\"id\":%d}\n", t->id);
    } 
    else if (strcmp(argv[1], "list_tickets") == 0) {
        int filterUid = (argc >= 3) ? atoi(argv[2]) : 0;
        printf("{\"success\":true,\"tickets\":[");
        int first = 1;
        for (Ticket* t = ticketHead; t; t = t->next) {
            if (filterUid == 0 || t->uid == filterUid) {
                if (!first) printf(",");
                printf("{\"id\":%d,\"status\":\"%s\",\"desc\":", t->id, statusNames[t->status]);
                printJsonString(t->description);
                printf(",\"priority\":%d}", t->priority);
                first = 0;
            }
        }
        printf("]}\n");
    }
    return 0;
}