// Enhanced Bank-account program reads a random-access file sequentially,
// updates data already written to the file, creates new data to
// be placed in the file, and deletes data previously in the file.
//
// New features added:
//   1. Search account by last name
//   2. Display all accounts summary (total balance + count)
//   3. Input validation (account range, duplicate prevention)
//   4. Transaction history log to "transactions.log"
//   5. Confirm before deleting a record
//   6. Show active account count on startup

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// clientData structure definition
struct clientData
{
    unsigned int acctNum; // account number
    char lastName[15];    // account last name
    char firstName[10];   // account first name
    double balance;       // account balance
};

// ── prototypes ──────────────────────────────────────────────────────────────
unsigned int enterChoice(void);
void textFile(FILE *readPtr);
void updateRecord(FILE *fPtr);
void newRecord(FILE *fPtr);
void deleteRecord(FILE *fPtr);

// ── NEW feature prototypes ───────────────────────────────────────────────────
void searchByName(FILE *fPtr);
void displaySummary(FILE *fPtr);
void logTransaction(unsigned int acct, const char *type, double amount, double newBalance);
int  countActiveAccounts(FILE *fPtr);

// ── constants ────────────────────────────────────────────────────────────────
#define MAX_ACCOUNTS 100

// ============================================================================
int main(int argc, char *argv[])
{
    FILE *cfPtr;
    unsigned int choice;

    if ((cfPtr = fopen("credit.dat", "rb+")) == NULL)
    {
        printf("%s: File could not be opened.\n", argv[0]);
        exit(-1);
    }

    // NEW: show active account count on startup
    int activeCount = countActiveAccounts(cfPtr);
    printf("\n=== Bank Account System ===\n");
    printf("Active accounts on file: %d / %d\n", activeCount, MAX_ACCOUNTS);
    printf("===========================\n");

    while ((choice = enterChoice()) != 7)   // menu now has 7 options
    {
        switch (choice)
        {
        case 1: textFile(cfPtr);      break;
        case 2: updateRecord(cfPtr);  break;
        case 3: newRecord(cfPtr);     break;
        case 4: deleteRecord(cfPtr);  break;
        case 5: searchByName(cfPtr);  break;   // NEW
        case 6: displaySummary(cfPtr); break;  // NEW
        default:
            puts("Incorrect choice. Please enter 1-7.");
            break;
        }
    }

    fclose(cfPtr);
    puts("\nGoodbye!");
    return 0;
}

// ============================================================================
// Count how many active (non-zero) accounts exist in the file
// ============================================================================
int countActiveAccounts(FILE *fPtr)
{
    struct clientData client;
    int count = 0;

    rewind(fPtr);
    while (fread(&client, sizeof(struct clientData), 1, fPtr) == 1)
    {
        if (client.acctNum != 0)
            count++;
    }
    return count;
}

// ============================================================================
// Log a transaction to transactions.log with a timestamp
// ============================================================================
void logTransaction(unsigned int acct, const char *type, double amount, double newBalance)
{
    FILE *logPtr = fopen("transactions.log", "a");
    if (logPtr == NULL)
    {
        puts("Warning: could not open transactions.log");
        return;
    }

    time_t now = time(NULL);
    char timeBuf[26];
    // Use ctime_s on MSVC, or ctime on POSIX
#ifdef _WIN32
    ctime_s(timeBuf, sizeof(timeBuf), &now);
#else
    char *tmp = ctime(&now);
    strncpy(timeBuf, tmp, sizeof(timeBuf) - 1);
    timeBuf[sizeof(timeBuf) - 1] = '\0';
#endif
    // Remove trailing newline from ctime
    timeBuf[strcspn(timeBuf, "\n")] = '\0';

    fprintf(logPtr, "[%s] Acct #%-3u | %-10s | Amount: %+10.2f | New Balance: %10.2f\n",
            timeBuf, acct, type, amount, newBalance);

    fclose(logPtr);
}

// ============================================================================
// Create formatted text file for printing
// ============================================================================
void textFile(FILE *readPtr)
{
    FILE *writePtr;
    int result;
    struct clientData client = {0, "", "", 0.0};

    if ((writePtr = fopen("accounts.txt", "w")) == NULL)
    {
        puts("File could not be opened.");
        return;
    }

    rewind(readPtr);
    fprintf(writePtr, "%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");
    fprintf(writePtr, "%-6s%-16s%-11s%10s\n", "----", "---------", "----------", "-------");

    while (!feof(readPtr))
    {
        result = fread(&client, sizeof(struct clientData), 1, readPtr);
        if (result != 0 && client.acctNum != 0)
        {
            fprintf(writePtr, "%-6d%-16s%-11s%10.2f\n",
                    client.acctNum, client.lastName, client.firstName, client.balance);
        }
    }

    fclose(writePtr);
    puts("accounts.txt has been written.");
}
// Update balance in record  (with transaction logging)
void updateRecord(FILE *fPtr)
{
    unsigned int account;
    double transaction;
    struct clientData client = {0, "", "", 0.0};

    printf("%s", "Enter account to update ( 1 - 100 ): ");
    scanf("%d", &account);

    // NEW: input validation
    if (account < 1 || account > MAX_ACCOUNTS)
    {
        printf("Invalid account number. Must be between 1 and %d.\n", MAX_ACCOUNTS);
        return;
    }

    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    {
        printf("Account #%d has no information.\n", account);
        return;
    }

    printf("%-6d%-16s%-11s%10.2f\n\n",
           client.acctNum, client.lastName, client.firstName, client.balance);

    printf("%s", "Enter charge ( + ) or payment ( - ): ");
    scanf("%lf", &transaction);

    // NEW: warn if transaction would overdraft
    if (client.balance + transaction < 0.0)
    {
        printf("Warning: this transaction would result in a negative balance (%.2f).\n",
               client.balance + transaction);
        printf("Proceed anyway? (y/n): ");
        char confirm;
        scanf(" %c", &confirm);
        if (confirm != 'y' && confirm != 'Y')
        {
            puts("Transaction cancelled.");
            return;
        }
    }

    client.balance += transaction;

    printf("%-6d%-16s%-11s%10.2f\n",
           client.acctNum, client.lastName, client.firstName, client.balance);

    fseek(fPtr, -(long)sizeof(struct clientData), SEEK_CUR);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);

    // NEW: log the transaction
    logTransaction(account,
                   transaction >= 0 ? "CHARGE" : "PAYMENT",
                   transaction,
                   client.balance);
    puts("Transaction logged.");
}

// Delete an existing record  (with confirmation prompt)
void deleteRecord(FILE *fPtr)
{
    struct clientData client;
    struct clientData blankClient = {0, "", "", 0};
    unsigned int accountNum;

    printf("%s", "Enter account number to delete ( 1 - 100 ): ");
    scanf("%d", &accountNum);

    
    if (accountNum < 1 || accountNum > MAX_ACCOUNTS)
    {
        printf("Invalid account number. Must be between 1 and %d.\n", MAX_ACCOUNTS);
        return;
    }

    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    {
        printf("Account %d does not exist.\n", accountNum);
        return;
    }

    
    printf("About to delete: %-6d%-16s%-11s%10.2f\n",
           client.acctNum, client.lastName, client.firstName, client.balance);
    printf("Are you sure? (y/n): ");
    char confirm;
    scanf(" %c", &confirm);

    if (confirm == 'y' || confirm == 'Y')
    {
        fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
        fwrite(&blankClient, sizeof(struct clientData), 1, fPtr);
        printf("Account #%d deleted.\n", accountNum);
        logTransaction(accountNum, "DELETE", 0.0, 0.0);
    }
    else
    {
        puts("Deletion cancelled.");
    }
}

void newRecord(FILE *fPtr)
{
    struct clientData client = {0, "", "", 0.0};
    unsigned int accountNum;

    printf("%s", "Enter new account number ( 1 - 100 ): ");
    scanf("%d", &accountNum);

    // NEW: input validation
    if (accountNum < 1 || accountNum > MAX_ACCOUNTS)
    {
        printf("Invalid account number. Must be between 1 and %d.\n", MAX_ACCOUNTS);
        return;
    }

    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum != 0)
    {
        printf("Account #%d already contains information.\n", client.acctNum);
        return;
    }

    printf("%s", "Enter lastname, firstname, balance\n? ");
    scanf("%14s%9s%lf", client.lastName, client.firstName, &client.balance);

    client.acctNum = accountNum;

    fseek(fPtr, (client.acctNum - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);

    printf("Account #%d created successfully.\n", accountNum);
    logTransaction(accountNum, "NEW ACCT", client.balance, client.balance);
}
// NEW: Search for accounts by last name (case-insensitive, partial match)
void searchByName(FILE *fPtr)
{
    char searchName[15];
    struct clientData client;
    int found = 0;

    printf("Enter last name to search: ");
    scanf("%14s", searchName);

    // Convert search term to lowercase for comparison
    for (int i = 0; searchName[i]; i++)
        if (searchName[i] >= 'A' && searchName[i] <= 'Z')
            searchName[i] += 32;

    printf("\n%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");
    printf("%-6s%-16s%-11s%10s\n", "----", "---------", "----------", "-------");

    rewind(fPtr);
    while (fread(&client, sizeof(struct clientData), 1, fPtr) == 1)
    {
        if (client.acctNum == 0)
            continue;

        // Copy and lowercase the record's last name for comparison
        char lname[15];
        strncpy(lname, client.lastName, 14);
        lname[14] = '\0';
        for (int i = 0; lname[i]; i++)
            if (lname[i] >= 'A' && lname[i] <= 'Z')
                lname[i] += 32;

        if (strstr(lname, searchName) != NULL)
        {
            printf("%-6d%-16s%-11s%10.2f\n",
                   client.acctNum, client.lastName, client.firstName, client.balance);
            found++;
        }
    }

    if (found == 0)
        printf("No accounts found matching \"%s\".\n", searchName);
    else
        printf("\n%d account(s) found.\n", found);
}

void displaySummary(FILE *fPtr)
{
    struct clientData client;
    int count = 0;
    double total = 0.0;
    double highest = -1e18, lowest = 1e18;
    unsigned int highAcct = 0, lowAcct = 0;

    rewind(fPtr);
    while (fread(&client, sizeof(struct clientData), 1, fPtr) == 1)
    {
        if (client.acctNum == 0)
            continue;
        count++;
        total += client.balance;
        if (client.balance > highest) { highest = client.balance; highAcct = client.acctNum; }
        if (client.balance < lowest)  { lowest  = client.balance; lowAcct  = client.acctNum; }
    }

    printf("====Account Summary====\n");
    if (count == 0)
    {
        puts("No active accounts.");
        return;
    }
    printf("Active accounts  : %d\n",     count);
    printf("Total balance    : $%.2f\n",  total);
    printf("Average balance  : $%.2f\n",  total / count);
    printf("Highest balance  : $%.2f  (Acct #%u)\n", highest, highAcct);
    printf("Lowest  balance  : $%.2f  (Acct #%u)\n", lowest,  lowAcct);
    printf("\n");
}


unsigned int enterChoice(void)
{
    unsigned int menuChoice;
    printf("%s", "\nEnter your choice\n"
                 "1 - store a formatted text file of accounts (accounts.txt)\n"
                 "2 - update an account\n"
                 "3 - add a new account\n"
                 "4 - delete an account\n"
                 "5 - search accounts by last name\n"     
                 "6 - display account summary\n"           
                 "7 - end program\n? ");
    scanf("%u", &menuChoice);
    return menuChoice;
}
