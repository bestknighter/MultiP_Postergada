#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct {
    char strVal[21];
    int intVal;
} tTuple;

typedef struct msgbuf {
	long mtype;
	char mtext[128];
} message_buf;

static struct { char strVal[21]; int intVal; } tuple[10];
static int tupleCount = 0;
//void delay(int number_of_seconds);
bool can_exec(const char *file);
static void listTuples(void);
static void addTuple(char *str, int val);
static void deleteTuple(char *str);
