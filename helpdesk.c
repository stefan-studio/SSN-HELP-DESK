#include "helpdesk.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static TicketNode* ticketBSTRoot = NULL;
static User     users[MAX_USERS];
static Engineer engineers[MAX_ENGINEERS];

static int ticketCount   = 0;
static int userCount     = 0;
static int engineerCount = 0;

const char *issueNames[] = { "Furniture", "WiFi", "Network", "Hardware", "Software", "Other" };
const char *statusNames[] = { "Open", "Assigned", "In Progress", "Resolved", "Closed" };

TicketNode* createNewTicketNode(Ticket newTicket) {
    TicketNode* newNode = (TicketNode*)malloc(sizeof(TicketNode));
    if (newNode != NULL) {
        newNode->data = newTicket;
        newNode->leftChild = NULL;
        newNode->rightChild = NULL;
    }
    return newNode;
}

TicketNode* insertTicketIntoBST(TicketNode* root, Ticket newTicket) {
    if (root == NULL) return createNewTicketNode(newTicket);
    if (newTicket.id < root->data.id) root->leftChild = insertTicketIntoBST(root->leftChild, newTicket);
    else if (newTicket.id > root->data.id) root->rightChild = insertTicketIntoBST(root->rightChild, newTicket);
    return root;
}

TicketNode* searchTicketInBST(TicketNode* root, int targetId) {
    if (root == NULL || root->data.id == targetId) return root;
    if (targetId < root->data.id) return searchTicketInBST(root->leftChild, targetId);
    return searchTicketInBST(root->rightChild, targetId);
}

int compareIgnoreCase(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return (*a == '\0' && *b == '\0');
}

const char *getIssueName(enum IssueType issue) { return issueNames[issue]; }

enum IssueType parseIssueType(const char *s) {
    for (int i = 0; i < (int)(sizeof(issueNames)/sizeof(issueNames[0])); i++)
        if (compareIgnoreCase(issueNames[i], s)) return (enum IssueType)i;
    return OTHER;
}

static void jsonEscapeStr(char *safeOut, const char *rawIn, int maxLen) {
    int outIndex = 0;
    for (int i = 0; rawIn[i] != '\0' && outIndex < maxLen - 4; i++) {
        switch (rawIn[i]) {
            case '"':  safeOut[outIndex++] = '\\'; safeOut[outIndex++] = '"';  break;
            case '\\': safeOut[outIndex++] = '\\'; safeOut[outIndex++] = '\\'; break;
            case '\n': safeOut[outIndex++] = '\\'; safeOut[outIndex++] = 'n';  break;
            case '\r': safeOut[outIndex++] = '\\'; safeOut[outIndex++] = 'r';  break;
            case '\t': safeOut[outIndex++] = '\\'; safeOut[outIndex++] = 't';  break;
            default:   safeOut[outIndex++] = rawIn[i]; break;
        }
    }
    safeOut[outIndex] = '\0';
}

static void fmtTime(char *timeBuffer, size_t bufferSize, time_t rawTime) {
    if (rawTime <= 0) { snprintf(timeBuffer, bufferSize, "null"); return; }
    struct tm *timeStruct = localtime(&rawTime);
    snprintf(timeBuffer, bufferSize, "\"%04d-%02d-%02dT%02d:%02d:%02d\"",
             timeStruct->tm_year + 1900, timeStruct->tm_mon + 1, timeStruct->tm_mday,
             timeStruct->tm_hour, timeStruct->tm_min, timeStruct->tm_sec);
}

static void printTicketJSON(const Ticket *ticket) {
    char safeDesc[MAX_DESCRIPTION_LEN * 2], safeNotes[MAX_NOTES_LEN * 2];
    char timeCreated[40], timeAssigned[40], timeClosed[40], engineerIdStr[16];

    jsonEscapeStr(safeDesc, ticket->description, sizeof(safeDesc));
    jsonEscapeStr(safeNotes, ticket->notes, sizeof(safeNotes));
    fmtTime(timeCreated, sizeof(timeCreated), ticket->timeCreated);
    fmtTime(timeAssigned, sizeof(timeAssigned), ticket->timeAssigned);
    fmtTime(timeClosed, sizeof(timeClosed), ticket->timeClosed);

    if (ticket->eid < 0) snprintf(engineerIdStr, sizeof(engineerIdStr), "null");
    else                 snprintf(engineerIdStr, sizeof(engineerIdStr), "%d", ticket->eid);

    printf("{\"id\":%d,\"user_id\":%d,\"engineer_id\":%s,\"issue_type\":\"%s\",\"status\":\"%s\",\"description\":\"%s\",\"notes\":\"%s\",\"created_at\":%s,\"assigned_at\":%s,\"closed_at\":%s,\"priority\":%d}",
           ticket->id, ticket->uid, engineerIdStr, issueNames[ticket->issueType], statusNames[ticket->status],
           safeDesc, safeNotes, timeCreated, timeAssigned, timeClosed, ticket->priority);
}

void printTicketsInOrder(TicketNode* root, int targetUid, int* isFirstElement) {
    if (root == NULL) return;
    printTicketsInOrder(root->leftChild, targetUid, isFirstElement);
    if (targetUid == 0 || root->data.uid == targetUid) {
        if (!(*isFirstElement)) printf(",");
        printTicketJSON(&(root->data));
        *isFirstElement = 0;
    }
    printTicketsInOrder(root->rightChild, targetUid, isFirstElement);
}

static void printEngineerJSON(const Engineer *e) {
    char safeName[MAX_USERNAME_LEN * 2];
    jsonEscapeStr(safeName, e->name, sizeof(safeName));
    printf("{\"id\":%d,\"name\":\"%s\",\"specialty\":\"%s\",\"active_tickets\":%d,\"total_resolved\":%d}",
           e->id, safeName, issueNames[e->specialty], e->ticketsAssigned, e->ticketsResolved);
}

static void loadTickets(void) {
    FILE *file = fopen("tickets.db", "rb");
    if (!file) return;
    int savedTicketCount = 0;
    fread(&savedTicketCount, sizeof(int), 1, file);
    for (int i = 0; i < savedTicketCount; i++) {
        Ticket tempTicket;
        if (fread(&tempTicket, sizeof(Ticket), 1, file) == 1) {
            ticketBSTRoot = insertTicketIntoBST(ticketBSTRoot, tempTicket);
            ticketCount++;
        }
    }
    fclose(file);
}

void writeTicketsToDiskRecursive(TicketNode* root, FILE* file) {
    if (root == NULL) return;
    writeTicketsToDiskRecursive(root->leftChild, file);
    fwrite(&(root->data), sizeof(Ticket), 1, file);
    writeTicketsToDiskRecursive(root->rightChild, file);
}

int syncTickets(void) {
    FILE *file = fopen("tickets.db", "wb");
    if (!file) return -1;
    fwrite(&ticketCount, sizeof(int), 1, file);
    writeTicketsToDiskRecursive(ticketBSTRoot, file);
    fclose(file);
    return 0;
}

static void loadEngineers(void) {
    FILE *f = fopen("engineers.db", "rb");
    if (!f) return;
    fread(&engineerCount, sizeof(int), 1, f);
    if (engineerCount < 0 || engineerCount > MAX_ENGINEERS) engineerCount = 0;
    fread(engineers, sizeof(Engineer), engineerCount, f);
    fclose(f);
}

static int syncEngineers(void) {
    FILE *f = fopen("engineers.db", "wb");
    if (!f) return -1;
    fwrite(&engineerCount, sizeof(int), 1, f);
    fwrite(engineers, sizeof(Engineer), engineerCount, f);
    fclose(f);
    return 0;
}

/* ── HEAP LOGIC FOR ADMIN TICKETS ── */
static void insertMinHeap(Ticket** heap, int* size, Ticket* t) {
    int i = *size;
    (*size)++;
    heap[i] = t;
    while (i != 0 && heap[(i - 1) / 2]->priority > heap[i]->priority) {
        Ticket* temp = heap[i];
        heap[i] = heap[(i - 1) / 2];
        heap[(i - 1) / 2] = temp;
        i = (i - 1) / 2;
    }
}

static Ticket* extractMin(Ticket** heap, int* size) {
    if (*size <= 0) return NULL;
    if (*size == 1) {
        (*size)--;
        return heap[0];
    }
    Ticket* root = heap[0];
    heap[0] = heap[*size - 1];
    (*size)--;

    int i = 0;
    while (1) {
        int smallest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < *size && heap[left]->priority < heap[smallest]->priority)
            smallest = left;
        if (right < *size && heap[right]->priority < heap[smallest]->priority)
            smallest = right;

        if (smallest != i) {
            Ticket* temp = heap[i];
            heap[i] = heap[smallest];
            heap[smallest] = temp;
            i = smallest;
        } else {
            break;
        }
    }
    return root;
}

static void collectTicketsForHeap(TicketNode* root, Ticket** heap, int* size) {
    if (root == NULL) return;
    collectTicketsForHeap(root->leftChild, heap, size);
    if (root->data.priority > 0 && root->data.priority <= 5) {
        insertMinHeap(heap, size, &(root->data));
    }
    collectTicketsForHeap(root->rightChild, heap, size);
}

static void printUnassignedTickets(TicketNode* root, int* isFirstElement) {
    if (root == NULL) return;
    printUnassignedTickets(root->leftChild, isFirstElement);
    if (root->data.priority <= 0 || root->data.priority > 5) {
        if (!(*isFirstElement)) printf(",");
        printTicketJSON(&(root->data));
        *isFirstElement = 0;
    }
    printUnassignedTickets(root->rightChild, isFirstElement);
}

/* ── UPDATED CREDENTIALS ── */
static void initUsersDefault(void) {
    users[0] = (User){1,  "user",  "pass", STAFF,   "Normal User"};
    users[1] = (User){2,  "admin", "pass", MANAGER, "Administrator"};
    users[2] = (User){3,  "middleman", "pass", MIDDLEMAN, "Ticket Middleman"};
    users[3] = (User){101, "ravi", "pass", ENGINEER, "Ravi Kumar"};
    userCount = 4;
}

static void initEngineersDefault(void) {
    engineers[0] = (Engineer){101, "Ravi Kumar",   WIFI,      0, 0};
    engineers[1] = (Engineer){102, "Priya Sharma", HARDWARE,  0, 0};
    engineers[2] = (Engineer){103, "Karthik N",    FURNITURE, 0, 0};
    engineers[3] = (Engineer){104, "Meena Iyer",   SOFTWARE,  0, 0};
    engineers[4] = (Engineer){105, "Arjun Das",    NETWORK,   0, 0};
    engineerCount = 5;
    syncEngineers();
}

int authenticateUser(char *username, char *password) {
    for (int i = 0; i < userCount; i++)
        if (compareIgnoreCase(users[i].username, username) &&
            strcmp(users[i].password, password) == 0)
            return i;
    return -1;
}

static int genTicketID(void) {
    int lastID = 0;
    FILE *f = fopen("ticket_counter.dat", "rb");
    if (f) { fread(&lastID, sizeof(int), 1, f); fclose(f); }
    lastID++;
    f = fopen("ticket_counter.dat", "wb");
    if (f) { fwrite(&lastID, sizeof(int), 1, f); fclose(f); }
    return lastID;
}

static void assignEngineer(Ticket *ticket) {
    int bestEngineerIndex = -1;
    int minimumWorkload = INT_MAX;

    for (int i = 0; i < engineerCount; i++) {
        if (engineers[i].specialty == ticket->issueType && engineers[i].ticketsAssigned < minimumWorkload) {
            minimumWorkload = engineers[i].ticketsAssigned;
            bestEngineerIndex = i;
        }
    }
    if (bestEngineerIndex == -1) {
        minimumWorkload = INT_MAX;
        for (int i = 0; i < engineerCount; i++) {
            if (engineers[i].ticketsAssigned < minimumWorkload) {
                minimumWorkload = engineers[i].ticketsAssigned;
                bestEngineerIndex = i;
            }
        }
    }
    if (bestEngineerIndex == -1) return;

    ticket->eid = engineers[bestEngineerIndex].id;
    ticket->status = ASSIGNED;
    ticket->timeAssigned = time(NULL);
    engineers[bestEngineerIndex].ticketsAssigned++;
}

Ticket *createTicket(int uid, const char *issueType, const char *description, int priority, const char *notes, const char *imagePath) {
    Ticket newTicket;
    newTicket.id = genTicketID();
    newTicket.uid = uid;
    newTicket.eid = -1;
    newTicket.issueType = parseIssueType(issueType);
    newTicket.status = OPEN;
    newTicket.priority = priority; // Set the inferred priority
    newTicket.timeCreated = time(NULL);
    newTicket.timeAssigned = 0;
    newTicket.timeClosed = 0;
    strncpy(newTicket.notes, notes, MAX_NOTES_LEN - 1); // Set the smart notes
    newTicket.notes[MAX_NOTES_LEN - 1] = '\0';
    strncpy(newTicket.imagePath, imagePath, 255);
    newTicket.imagePath[255] = '\0';

    strncpy(newTicket.description, description, MAX_DESCRIPTION_LEN - 1);
    newTicket.description[MAX_DESCRIPTION_LEN - 1] = '\0';
    ticketBSTRoot = insertTicketIntoBST(ticketBSTRoot, newTicket);
    ticketCount++;
    syncTickets();
    syncEngineers();

    TicketNode* insertedNode = searchTicketInBST(ticketBSTRoot, newTicket.id);
    return &(insertedNode->data);
}

void closeTicket(Ticket *ticket) {
    if (ticket->status == CLOSED) return;
    ticket->status = CLOSED;
    ticket->timeClosed = time(NULL);
    for (int i = 0; i < engineerCount; i++) {
        if (engineers[i].id == ticket->eid) {
            engineers[i].ticketsResolved++;
            if (engineers[i].ticketsAssigned > 0) engineers[i].ticketsAssigned--;
            break;
        }
    }
    syncTickets();
    syncEngineers();
}

static int reassignTicket(int ticketId, int manualEid) {
    TicketNode* targetNode = searchTicketInBST(ticketBSTRoot, ticketId);
    if (targetNode != NULL) {
        Ticket* ticket = &(targetNode->data);
        // Release old engineer
        for (int j = 0; j < engineerCount; j++) {
            if (engineers[j].id == ticket->eid) {
                if (engineers[j].ticketsAssigned > 0) engineers[j].ticketsAssigned--;
                break;
            }
        }

        if (manualEid > 0) {
            ticket->eid = manualEid;
            ticket->status = ASSIGNED;
            ticket->timeAssigned = time(NULL);
            for(int i=0; i<engineerCount; i++) if(engineers[i].id == manualEid) engineers[i].ticketsAssigned++;
        } else {
            ticket->eid = -1;
            ticket->status = OPEN;
            assignEngineer(ticket);
        }
        syncTickets();
        syncEngineers();
        return 0;
    }
    return -1;
}

static void cmdLogin(int argc, char *argv[]) {
    if (argc < 4) {
        printf("{\"success\":false,\"error\":\"Usage: login <username> <password>\"}\n");
        exit(1);
    }
    int idx = authenticateUser(argv[2], argv[3]);
    if (idx < 0) {
        printf("{\"success\":false,\"error\":\"Invalid credentials\"}\n");
        exit(1);
    }
    char safeUname[MAX_USERNAME_LEN * 2];
    char safeFullname[MAX_USERNAME_LEN * 2];
    jsonEscapeStr(safeUname, users[idx].username, sizeof(safeUname));
    jsonEscapeStr(safeFullname, users[idx].name, sizeof(safeFullname));
    const char *role;
    if (users[idx].role == MANAGER) role = "admin";
    else if (users[idx].role == MIDDLEMAN) role = "middleman";
    else role = "user";
    printf("{\"success\":true,\"user_id\":%d,\"username\":\"%s\",\"full_name\":\"%s\",\"role\":\"%s\"}\n",
           users[idx].id, safeUname, safeFullname, role);
    exit(0);
}

static void cmdCreateTicket(int argc, char *argv[]) {
    // argc[0]=binary, [1]=cmd, [2]=uid, [3]=type, [4]=desc, [5]=prio, [6]=notes, [7]=img
    if (argc < 8) {
        printf("{\"success\":false,\"error\":\"Insufficient arguments for create_ticket\"}\n");
        exit(1);
    }
    int uid = atoi(argv[2]);
    const char *issueType = argv[3];
    const char *description = argv[4];
    int priority = atoi(argv[5]);
    const char *notes = argv[6];
    const char *imagePath = argv[7];

    Ticket *t = createTicket(uid, issueType, description, priority, notes, imagePath);
    if (!t) {
        printf("{\"success\":false,\"error\":\"System error\"}\n");
        exit(1);
    }
    printf("{\"success\":true,\"ticket\":");
    printTicketJSON(t);
    printf("}\n");
    exit(0);
}

static void cmdCloseTicket(int argc, char *argv[]) {
    if (argc < 3) {
        printf("{\"success\":false,\"error\":\"Usage: close_ticket <ticket_id>\"}\n");
        exit(1);
    }
    int tid = atoi(argv[2]);
    TicketNode* targetNode = searchTicketInBST(ticketBSTRoot, tid);
    if (targetNode != NULL) {
        if (targetNode->data.status == CLOSED) {
            printf("{\"success\":false,\"error\":\"Ticket already closed\"}\n");
            exit(1);
        }
        closeTicket(&(targetNode->data));
        printf("{\"success\":true,\"message\":\"Ticket #%d closed\",\"ticket\":", tid);
        printTicketJSON(&(targetNode->data));
        printf("}\n");
        exit(0);
    }
    printf("{\"success\":false,\"error\":\"Ticket #%d not found\"}\n", tid);
    exit(1);
}

static void cmdAssignTicket(int argc, char *argv[]) {
    if (argc < 3) {
        printf("{\"success\":false,\"error\":\"Usage: assign_ticket <ticket_id> [engineer_id]\"}\n");
        exit(1);
    }
    int tid = atoi(argv[2]);
    int eid = (argc >= 4) ? atoi(argv[3]) : -1;
    int rc  = reassignTicket(tid, eid);
    if (rc != 0) {
        printf("{\"success\":false,\"error\":\"Ticket #%d not found\"}\n", tid);
        exit(1);
    }
    TicketNode* updatedNode = searchTicketInBST(ticketBSTRoot, tid);
    printf("{\"success\":true,\"ticket\":");
    printTicketJSON(&(updatedNode->data));
    printf("}\n");
    exit(0);
}

static void cmdSetPriority(int argc, char *argv[]) {
    if (argc < 4) {
        printf("{\"success\":false,\"error\":\"Usage: set_priority <ticket_id> <priority>\"}\n");
        exit(1);
    }
    int tid = atoi(argv[2]);
    int prio = atoi(argv[3]);
    TicketNode* targetNode = searchTicketInBST(ticketBSTRoot, tid);
    if (targetNode != NULL) {
        targetNode->data.priority = prio;
        syncTickets();
        printf("{\"success\":true,\"ticket\":");
        printTicketJSON(&(targetNode->data));
        printf("}\n");
        exit(0);
    }
    printf("{\"success\":false,\"error\":\"Ticket #%d not found\"}\n", tid);
    exit(1);
}

static void cmdAutoAssignTicket(int argc, char *argv[]) {
    if (argc < 3) {
        printf("{\"success\":false,\"error\":\"Usage: auto_assign_ticket <ticket_id>\"}\n");
        exit(1);
    }
    int tid = atoi(argv[2]);
    TicketNode* targetNode = searchTicketInBST(ticketBSTRoot, tid);
    if (targetNode != NULL) {
        assignEngineer(&(targetNode->data)); // Use the existing auto-assignment logic
        syncTickets();
        syncEngineers();
        printf("{\"success\":true,\"ticket\":");
        printTicketJSON(&(targetNode->data));
        printf("}\n");
        exit(0);
    }
    printf("{\"success\":false,\"error\":\"Ticket #%d not found\"}\n", tid);
    exit(1);
}

static void cmdSetNotes(int argc, char *argv[]) {
    if (argc < 4) {
        printf("{\"success\":false,\"error\":\"Usage: set_notes <ticket_id> <notes...>\"}\n");
        exit(1);
    }
    int tid = atoi(argv[2]);
    TicketNode* targetNode = searchTicketInBST(ticketBSTRoot, tid);
    if (targetNode != NULL) {
        targetNode->data.notes[0] = '\0';
        for (int i = 3; i < argc; i++) {
            if (i > 3) strncat(targetNode->data.notes, " ", MAX_NOTES_LEN - strlen(targetNode->data.notes) - 1);
            strncat(targetNode->data.notes, argv[i], MAX_NOTES_LEN - strlen(targetNode->data.notes) - 1);
        }
        syncTickets();
        printf("{\"success\":true}\n");
        exit(0);
    }
    printf("{\"success\":false,\"error\":\"Ticket #%d not found\"}\n", tid);
    exit(1);
}

static void cmdSearchBST(int argc, char *argv[]) {
    if (argc < 3) {
        printf("{\"success\":false,\"error\":\"Usage: search_bst <ticket_id>\"}\n");
        exit(1);
    }
    int tid = atoi(argv[2]);
    int path[100];
    int pathSize = 0;

    TicketNode* curr = ticketBSTRoot;
    while (curr != NULL) {
        path[pathSize++] = curr->data.id;
        if (curr->data.id == tid) break;
        if (tid < curr->data.id) curr = curr->leftChild;
        else curr = curr->rightChild;
    }

    printf("{\"success\":true,\"found\":%s,\"path\":[", (curr != NULL) ? "true" : "false");
    for (int i = 0; i < pathSize; i++) {
        printf("%d%s", path[i], (i == pathSize - 1) ? "" : ",");
    }
    printf("]");

    if (curr != NULL) {
        printf(",\"ticket\":");
        printTicketJSON(&(curr->data));
    }

    printf("}\n");
    exit(0);
}

static void cmdEditTicket(int argc, char *argv[]) {
    if (argc < 5) {
        printf("{\"success\":false,\"error\":\"Usage: edit_ticket <uid> <tid> <new_desc>\"}\n");
        exit(1);
    }
    int uid = atoi(argv[2]);
    int tid = atoi(argv[3]);
    const char *newDesc = argv[4];

    TicketNode* node = searchTicketInBST(ticketBSTRoot, tid);
    if (node && node->data.uid == uid && node->data.status == OPEN) {
        strncpy(node->data.description, newDesc, MAX_DESCRIPTION_LEN - 1);
        node->data.description[MAX_DESCRIPTION_LEN - 1] = '\0';
        syncTickets();
        printf("{\"success\":true,\"ticket\":");
        printTicketJSON(&(node->data));
        printf("}\n");
        exit(0);
    }
    printf("{\"success\":false,\"error\":\"Ticket not found or unauthorized\"}\n");
    exit(1);
}

static void cmdDeleteTicket(int argc, char *argv[]) {
    if (argc < 4) {
        printf("{\"success\":false,\"error\":\"Usage: delete_ticket <uid> <tid>\"}\n");
        exit(1);
    }
    int uid = atoi(argv[2]);
    int tid = atoi(argv[3]);

    TicketNode* node = searchTicketInBST(ticketBSTRoot, tid);
    if (node && node->data.uid == uid && node->data.status == OPEN) {
        // Note: For a production delete, you'd implement deleteTicketFromBST logic.
        // Here, we just mark as Closed or implement node removal if BST delete is available.
        node->data.status = CLOSED; 
        syncTickets();
        printf("{\"success\":true}\n");
        exit(0);
    }
    printf("{\"success\":false,\"error\":\"Ticket not found or unauthorized\"}\n");
    exit(1);
}

static void cmdListTicketsAdmin(void) {
    Ticket* heap[MAX_TICKETS_HEAP];
    int heapSize = 0;
    collectTicketsForHeap(ticketBSTRoot, heap, &heapSize);

    int isFirstElement = 1;
    printf("{\"success\":true,\"tickets\":[");

    while (heapSize > 0) {
        Ticket* t = extractMin(heap, &heapSize);
        if (!isFirstElement) printf(",");
        printTicketJSON(t);
        isFirstElement = 0;
    }

    printUnassignedTickets(ticketBSTRoot, &isFirstElement);
    
    printf("]}\n");
    exit(0);
}

static void cmdListTickets(int argc, char *argv[]) {
    int targetUid = (argc >= 3) ? atoi(argv[2]) : 0;
    int isFirstElement = 1;
    printf("{\"success\":true,\"tickets\":[");
    printTicketsInOrder(ticketBSTRoot, targetUid, &isFirstElement);
    printf("]}\n");
    exit(0);
}

static void cmdListEngineers(void) {
    printf("{\"success\":true,\"engineers\":[");
    for (int i = 0; i < engineerCount; i++) {
        if (i > 0) printf(",");
        printEngineerJSON(&engineers[i]);
    }
    printf("]}\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    initUsersDefault();
    loadEngineers();
    if (engineerCount == 0) initEngineersDefault();
    loadTickets();

    if (argc == 1) return 0;

    const char *cmd = argv[1];
    if      (strcmp(cmd, "login")          == 0) cmdLogin(argc, argv);
    else if (strcmp(cmd, "create_ticket")  == 0) cmdCreateTicket(argc, argv);
    else if (strcmp(cmd, "close_ticket")   == 0) cmdCloseTicket(argc, argv);
    else if (strcmp(cmd, "assign_ticket")  == 0) cmdAssignTicket(argc, argv);
    else if (strcmp(cmd, "set_priority")   == 0) cmdSetPriority(argc, argv);
    else if (strcmp(cmd, "set_notes")      == 0) cmdSetNotes(argc, argv);
    else if (strcmp(cmd, "edit_ticket")    == 0) cmdEditTicket(argc, argv);
    else if (strcmp(cmd, "delete_ticket")  == 0) cmdDeleteTicket(argc, argv);
    else if (strcmp(cmd, "auto_assign_ticket") == 0) cmdAutoAssignTicket(argc, argv);
    else if (strcmp(cmd, "list_tickets")   == 0) cmdListTickets(argc, argv);
    else if (strcmp(cmd, "search_bst")     == 0) cmdSearchBST(argc, argv);
    else if (strcmp(cmd, "list_tickets_admin") == 0) cmdListTicketsAdmin();
    else if (strcmp(cmd, "list_engineers") == 0) cmdListEngineers();
    else {
        printf("{\"success\":false,\"error\":\"Unknown command\"}\n");
        return 1;
    }
    return 0;
}