/* Heltool - a network test tool to be run in a container on Westermo products 
 * Copyright Henrik Envall 2022
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

// ************ Defines ***********

/// Packet payload size
#define PAYLOAD_SIZE 1024
/// Default port
#define HELTOOL_PORT 33333
/// Default MC group to use
#define HELTOOL_GROUP "224.0.2.15" //unassigned in the adhoc1 block
#define MAX_COUNT 1000000000UL                           
/// Reporting default interval 10 seconds 
#define HELTOOL_REPORT 10 

// ************ Functions ***********
void print_help(void);
int fill_buffer(char *buffer, int len);


