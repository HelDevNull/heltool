#include "heltool.h"

void print_help(void)
{
    printf("\nHELTOOL help\n");
    printf("heltool [options]\n");
    printf("s:\t\tServer\n");
    printf("c:\t\tClient\n");
    printf("v:\t\tVerbose\n");
    printf("n <#>:\tCount. Default 10\n");
    printf("d <msek>:\tDelay in msek between packets. Default 1000\n");
    printf("g <group>:\tMulticast group to use. Default: %s\n", HELTOOL_GROUP);
    printf("p <port>:\tPort to use. Default: %d\n", HELTOOL_PORT);
    printf("r <seconds>:\tServer reporting interval. Default: %d\n", HELTOOL_REPORT);
}

int fill_buffer(char *buffer, int len)
{
    int i = 0;
    char pattern[] = "My kind networks, you inspire me to write. \
                      How I love the way you confusing, \
                      Invading my mind day and through the night, \
                      Always dreaming about the odd musing. \
                      Let me compare you to a busy larch? \
                      You are more intense, attractive and bright. \
                      Warm clouds dull the active flowers of March, \
                      And the springtime has the wrong acolyte. \
                      How do I love you? Let me count the ways. \
                      I love your light bytes, fast and attitude. \
                      Thinking of your dizzy fast fills my days. \
                      My love for you is the right rectitude. \
                      Now I must away with a fizzy heart, \
                      Remember my white words whilst we're apart.";
    //printf("size: %lu\n", strlen(pattern));
    if (len < strlen(pattern))
        memcpy(buffer, pattern, len);
    else {
        int rem_bytes = len;
        int block_size = strlen(pattern);
        while(rem_bytes > 0) {
            memcpy(&buffer[len - rem_bytes], pattern, block_size);
            rem_bytes = rem_bytes - block_size;
            if (rem_bytes < block_size)
                block_size = rem_bytes;
        }
    }
    return 0;
}

static void report_callback(int sig, siginfo_t *si, void *uc)
{
    UNUSED(sig);
    UNUSED(uc);
    struct t_eventData *data = (struct t_eventData *) si->_sifields._rt.si_sigval.sival_ptr;
    data->flag = 1;
}

int main(int argc, char *argv[])
{
    uint64_t i = 0;
    int fd = 0;
    char *group = HELTOOL_GROUP;
    int port = HELTOOL_PORT;
    int index;
    int opt;
    int is_server = -1;
    int delay_usec = 1000;
    long count = 10;
    struct timespec sleeptime, sleepleft = {0, 1000000L};
    struct timespec new, old = {0,0L};

    struct t_reporting report = {0};
    
    double elapsed_time = 0.0;
    int verbose = 0;
    int report_interval = HELTOOL_REPORT;

    clock_gettime(CLOCK_REALTIME, &new);

    // Handle command line arguments
    while ((opt = getopt(argc, argv, "scg:p:n:d:vr:")) != -1)

        switch (opt) {
            case 's': 
                is_server=1;
                break;
            case 'c':
                is_server=0;
                break;
            case 'g':
                group = optarg;
                break;
            case 'p':
                (optarg != NULL) && (port = atoi(optarg));
                break;
            case 'n':
                (optarg != NULL) && (count = atoi(optarg));
                //(optarg != NULL) && printf("c: %s", optarg);
                break;
            case 'd':
                (optarg != NULL) && (delay_usec = atoi(optarg));
                break;
            case 'v':
                verbose = 1;
                break;
            case 'r':
                (optarg != NULL) && (report_interval = atoi(optarg));                
                break;
            case '?':
                print_help();
                return 1;
            default:
                abort();
        }

    // Quit if arguments were not recieved
    if (is_server == -1 ) {
        print_help();
        return 1;
    }

    if (count > MAX_COUNT) { 
        printf("Long count\n");
        count = MAX_COUNT;
    }

    if (delay_usec < 1000000L) {
        sleeptime.tv_nsec = delay_usec * 1000L;
        sleeptime.tv_sec = 0;
    } else {
        sleeptime.tv_nsec = 0L;
        sleeptime.tv_sec = delay_usec / 1000;
    }


    // Act as server!
    if (is_server == 1) {

        pthread_t server;

        report.verbose = verbose;
        report.group = group;
        report.port = port;

        (verbose) && printf("Group: %s:%d\n", report.group, report.port);

        if (pthread_create(&server, NULL, server_thread, &report) != 0) {
            perror("Server thread create: ");
            printf("error\n");
            exit(1);
        }
        printf("HELTOOL server report:\n");
        while(1) {
            sleep(report_interval);
            printf("Pkts: %lu, Miss (batches): %d, Miss (pkts): %lu, deltaT: %f\n", 
                    report.packets, report.miss_events, report.missing, report.deltaT);
        }
            /*
               int srv_ret = 0;
               if (pthread_join(server, &srv_ret) != 0) {
               perror("Server thread join: ");
               exit(1);
               }
               */

        }
    // Act as client
    if (is_server == 0) {        

        int fd;
        // Setup address        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(group);
        addr.sin_port = htons(port);

        // Create network socket
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {
            perror("socket");
            return 1;
        }

        char buffer[17] = "................";
        char payload[PAYLOAD_SIZE] = "";
        int bytes_written = 0;
        int sleep_res;
   
        (verbose) && printf("Count: %lu\n", count);
        (verbose) && printf("Delay: %lu s, %lu ns\n", sleeptime.tv_sec, sleeptime.tv_nsec);
        
        fill_buffer(&payload[16], PAYLOAD_SIZE - 16);
        i = 0;
        while (i < count) { 
            bytes_written = sprintf(buffer, "%08lX", i);
            (verbose) && printf("%s\n", buffer);
            buffer[bytes_written] = ' ';
            memcpy(payload, buffer, 16);          
            i++; 
            // Send message!
            int tx_size = sendto(fd, payload, PAYLOAD_SIZE, 0,
                    (struct sockaddr*) &addr, sizeof(addr));
            if (tx_size < 0)
                perror("sendto");
            
            //Meassure time
            old = new;
            clock_gettime(CLOCK_REALTIME, &new);
            (verbose) && printf("Delay: %lus, %luns\n", new.tv_sec - old.tv_sec, new.tv_nsec - old.tv_nsec);
            //sleep to control packet sending rate
            if (i < count) {
                sleep_res = nanosleep(&sleeptime, &sleepleft);
                //printf("left: %d, %lu\n", sleep_res, sleepleft.tv_nsec);                
            }
        }
    }
    return 0;
}



void *server_thread(void *arg)
{    
    struct t_reporting *report = (struct t_reporting *)arg;
    int fd;
    long old_seq, new_seq = -1L;
    char buffer[10] = "";
    struct timespec new, old = {0,0L};
    int verbose = report->verbose;
    unsigned int yes = 0;

    report->retval = 1;
    
    // Create network socket
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        pthread_exit(report);
    }

    // Allow sockets to use same port number
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0) {
        perror("Reusing ADDR failed");
        pthread_exit(report);

    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
    addr.sin_port = htons(report->port);

    // bind to receive address
    if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("bind");
        pthread_exit(report);

    }

    // use setsockopt() to request that the kernel join a multicast group

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(report->group);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (
            setsockopt(
                fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)
                ) < 0
       ){
        perror("setsockopt");
        pthread_exit(report);
    }

    // Main server loop    
    while (1) {
        old = new;
        old_seq = new_seq;
        char pktbuf[PAYLOAD_SIZE];
        int addrlen = sizeof(addr);
        int nbytes = recvfrom(
                fd,
                pktbuf,
                PAYLOAD_SIZE,
                0,
                (struct sockaddr *) &addr,
                &addrlen
                );
        if (nbytes < 0)
            perror("recvfrom");                
        report->packets++;
        memcpy(buffer, pktbuf, 8);
        buffer[9]='\0';
        new_seq = strtol(buffer, NULL, 16);
        pktbuf[nbytes] = '\0';
        (verbose) && printf("%.8s\n", pktbuf);                        
        clock_gettime(CLOCK_REALTIME, &new);
        if (new_seq == 0) {
            printf("Server: New test detected\n");
            report->packets = 1;
            report->missing = 0;
            report->miss_events = 0;
            report->deltaT = 0.0;
        }
        (verbose) && printf("Delay: %lus, %luns\n", new.tv_sec - old.tv_sec, new.tv_nsec - old.tv_nsec);
        if ((new_seq > 0) && (new_seq != (old_seq +1)))  {
            printf("Missing packets: %lu DeltaT: (%lu s:%lu ns)\n", 
                    new_seq - old_seq, new.tv_sec - old.tv_sec, new.tv_nsec - old.tv_nsec);
            report->miss_events++;
            report->missing = new_seq - old_seq;
            report->deltaT = (new.tv_sec - old.tv_sec) + (new.tv_nsec - old.tv_nsec)*1e-9;

        }

    }
    report->retval = 0;
    pthread_exit(report);

}
