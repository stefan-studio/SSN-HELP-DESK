// Connected to Jira SCRUM-4
#include "helpdesk.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global Queue instance
TicketQueue myQueue = {NULL, NULL};

// Constants for categorization (SCRUM-4)
const char* categories[] = {"Hostel", "Department", "General", "Other"};

// --- SYLLABUS CORE: QUEUE OPERATIONS ---

void initQueue(TicketQueue* q) {
    q->front = q->rear = NULL;
}

void viewHistory(TicketQueue* q, int studentID) {
    Ticket* temp = q->front;
    printf("\n--- Ticket History for Student ID: %d ---", studentID);
    
    int found = 0;
    while (temp != NULL) {
        if (temp->uid == studentID) {
            printf("\n[%s] Ticket #%d: %s | Status: %s", 
                    temp->category, temp->id, temp->description, temp->status);
            found = 1;
        }
        temp = temp->next; // Move to the next node in the Linked List
    }
    
    if (!found) printf("\nNo tickets found.");
    printf("\n---------------------------------------");
}

void displayAllTickets(TicketQueue* q) {
    Ticket* temp = q->front; // Start at the beginning
    
    if (temp == NULL) {
        printf("\n[Info] No active tickets in the system.\n");
        return;
    }

    printf("\n--- SYSTEM TICKET REPORT ---");
    while (temp != NULL) {
        printf("\nID: %d | Category: %s | Status: %s", 
                temp->id, temp->category, temp->status);
        printf("\nDescription: %s", temp->description);
        printf("\n----------------------------");
        
        temp = temp->next; // The "Link" part of Linked List
    }
}

// SCRUM-9: Ticket Creation Logic (Enqueue)
void enqueue(TicketQueue* q, int uid, char* cat, char* desc, char* file) {
    // 1. Dynamic Allocation (Syllabus: malloc)
    Ticket* newNode = (Ticket*)malloc(sizeof(Ticket));
    if (!newNode) {
        printf("Memory Error!\n");
        return;
    }

    // 2. Data Initialization
    newNode->id = genTicketID();
    newNode->uid = uid;
    strncpy(newNode->category, cat, 29);
    strncpy(newNode->description, desc, 255);
    strncpy(newNode->evidencePath, file, 255);
    strcpy(newNode->status, "Open");
    newNode->timeCreated = time(NULL);
    newNode->next = NULL;

    // 3. Queue Logic (Linking the nodes)
    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

// --- UTILITY FUNCTIONS ---

int genTicketID() {
    int lastID = 0;
    FILE* f = fopen("ticket_counter.dat", "rb");
    if (f) { fread(&lastID, sizeof(int), 1, f); fclose(f); }
    lastID++;
    f = fopen("ticket_counter.dat", "wb");
    if (f) { fwrite(&lastID, sizeof(int), 1, f); fclose(f); }
    return lastID;
}

// --- MODULE A: SUBMISSION UI HANDLER ---

int main() {
    int choice;
    int demoUserID = 2510161; 
    char catInput[30];
    char descInput[256];
    char fileInput[256];

    initQueue(&myQueue);

    while (1) {
        printf("\n====================================");
        printf("\n   SSN IT HELPDESK - MODULE A      ");
        printf("\n====================================");
        printf("\n1. Raise a New Ticket (Submission)");
        printf("\n2. View Queue (Linked List Traversal)");
        printf("\n3. Exit System");
        printf("\nChoose an option: ");
        
        if (scanf("%d", &choice) != 1) break;
        getchar(); // Clear buffer

        switch(choice) {
            case 1:
                printf("\nEnter Category (Hostel/Dept/General): ");
                fgets(catInput, 30, stdin);
                catInput[strcspn(catInput, "\n")] = 0;

                printf("Enter Description: ");
                fgets(descInput, 256, stdin);
                descInput[strcspn(descInput, "\n")] = 0;

                printf("Upload Evidence Path (SCRUM-5): ");
                fgets(fileInput, 256, stdin);
                fileInput[strcspn(fileInput, "\n")] = 0;

                enqueue(&myQueue, demoUserID, catInput, descInput, fileInput);
                printf("\n✅ TICKET SUBMITTED SUCCESSFULLY!");
                break;

            case 2:
                // Syllabus: Linked List Traversal
                printf("\n--- CURRENT TICKET QUEUE ---");
                Ticket* temp = myQueue.front;
                if (!temp) printf("\nQueue is empty.");
                while (temp) {
                    printf("\nID: %d | Cat: %s | Status: %s", temp->id, temp->category, temp->status);
                    temp = temp->next;
                }
                printf("\n----------------------------");
                break;

            case 3:
                printf("\nExiting...\n");
                return 0;

            default:
                printf("\nInvalid choice!");
        }
    }
    return 0;
}