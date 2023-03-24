/* Heltool - a network test tool to be run in a container on Westermo products 
 * Copyright Henrik Envall 2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <getopt.h>
#include <string.h>

#include <signal.h>
#include <time.h>
#include <errno.h>

#include <pthread.h>

// ************ Defines ***********

//Unused macro
#define UNUSED(x) (void)(x)

/// Packet payload size
#define PAYLOAD_SIZE 1024
/// Default port
#define HELTOOL_PORT 33333
/// Default MC group to use
#define HELTOOL_GROUP "224.0.2.15" //unassigned in the adhoc1 block
#define MAX_COUNT 1000000000UL                           
/// Reporting default interval 10 seconds 
#define HELTOOL_REPORT 10 


// *********** Structs ************

struct t_eventData{
    int flag;
};

struct t_reporting{
    int verbose;
    char *group;
    int port;
    long packets;
    int miss_events;
    long missing;
    double deltaT;
    int retval;
};

// ************ Functions ***********
void print_help(void);
int fill_buffer(char *buffer, int len);
static void report_callback(int sig, siginfo_t *si, void *uc);

void *server_thread(void *arg);
