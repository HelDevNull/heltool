#include "heltool-mc.h"

void print_help(void)
{
    printf("\nHELTOOL help\n");
    printf("heltool [options]\n");
    printf("s:\t\tServer\n");
    printf("c:\t\tClient\n");
    printf("v:\t\tVerbose\n");
    printf("n <#>:\t\tCount. Default %d\n", HELTOOL_COUNT);
    printf("d <msek>:\tDelay in msek between packets. Default %d\n", HELTOOL_INTERVAL);
    printf("g <group>:\tMulticast group to use. Default: %s\n", HELTOOL_GROUP);
    printf("p <port>:\tPort to use. Default: %d\n", HELTOOL_PORT);
    printf("r <seconds>:\tServer reporting interval. Default: %d\n", HELTOOL_REPORT);
}

int fill_buffer(char *buffer, int len)
{
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

int main(int argc, char *argv[])
{
    char *group = HELTOOL_GROUP;
    int port = HELTOOL_PORT;
    int opt;
    int is_server = -1;
    int delay_msec = HELTOOL_INTERVAL;
    long count = HELTOOL_COUNT;
    struct timespec sleeptime;
    struct t_reporting report = {0};
    int verbose = 0;
    int report_interval = HELTOOL_REPORT;

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
                (optarg != NULL) && (delay_msec = atoi(optarg));
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

    /// Convert sleeptime between packets to timespec nanosecond format
    if (delay_msec < 1000L) {
        sleeptime.tv_nsec = delay_msec * 1000000L;
        sleeptime.tv_sec = 0;
    } else {
        sleeptime.tv_nsec = 0L;
        sleeptime.tv_sec = delay_msec / 1000;
    }
    report.sleeptime = sleeptime;

    // Act as server!
    if (is_server == 1) {

        pthread_t server;

        report.verbose = verbose;
        report.group = group;
        report.port = port;

        (verbose) && printf("Server. Group: %s:%d\n", report.group, report.port);

        if (pthread_create(&server, NULL, server_thread, &report) != 0) {
            perror("Server thread create: ");
            printf("error\n");
            exit(1);
        }
        sleep(1);
        printf("HELTOOL server report:\n");
        printf("---------------------\n");
        printf("Total packets\tBatch\tLost pkts\tLost batches\tLast deltaT\n");
        while(1) {
            sleep(report_interval);
            printf("%lu\t\t%lu\t%lu\t\t\%d\t\t%f\n", report.packets, report.packet_batch,
                    report.missing, report.miss_events, report.deltaT);
        }


    }
    // Act as client
    if (is_server == 0) {        

        int ret;        
        pthread_t client;

        report.verbose = verbose;
        report.group = group;
        report.port = port;
        report.count = count;

        (verbose) && printf("Client. Group: %s:%d\n", report.group, report.port);

        if (pthread_create(&client, NULL, client_thread, &report) != 0) {
            perror("Client thread create: ");
            printf("error\n");
            exit(1);
        }
        printf("HELTOOL client report:\n");
        while(1) {
            ret = pthread_tryjoin_np(client, NULL);
            if (ret == 0) {
                printf("Client done. Sent %lu pkts\n", report.packets);
                break; // client thread has terminated
            }
            sleep(1);
        }



    }
    return 0;
}


void *client_thread(void *arg)
{
    int fd;
    struct t_reporting *report = (struct t_reporting *)arg;
    struct timespec sleepleft;
    struct timespec new, old = {0,0L};
    // Setup address        
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(report->group);
    addr.sin_port = htons(report->port);

    // Create network socket
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        pthread_exit(report);
    }

    char buffer[17] = "................";
    char payload[PAYLOAD_SIZE] = "";
    int bytes_written = 0;

    (report->verbose) && printf("Count: %lu\n", report->count);
    (report->verbose) && printf("Delay: %lu s, %lu ns\n", report->sleeptime.tv_sec, report->sleeptime.tv_nsec);

    fill_buffer(&payload[16], PAYLOAD_SIZE - 16);
    printf("Client delay: %lu, %lu\n", report->sleeptime.tv_sec, report->sleeptime.tv_nsec);
    printf("Client ready: %s:%d\n", report->group, report->port);
    long i = 0;
    while (i < report->count) { 
        bytes_written = sprintf(buffer, "%08lX", i);
        (report->verbose) && printf("%s\n", buffer);
        buffer[bytes_written] = ' ';
        memcpy(payload, buffer, 16);          
        i++; 
        // Send message!
        int tx_size = sendto(fd, payload, PAYLOAD_SIZE, 0,
                (struct sockaddr*) &addr, sizeof(addr));
        if (tx_size < 0)
            perror("sendto");
        report->packets++;
        //Meassure time
        old = new;
        clock_gettime(CLOCK_REALTIME, &new);
        (report->verbose) && printf("Delay: %lus, %luns\n", new.tv_sec - old.tv_sec, new.tv_nsec - old.tv_nsec);
        //sleep to control packet sending rate
        if (i < report->count) {
            nanosleep(&(report->sleeptime), &sleepleft);
        }
    }
    pthread_exit(report);
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

    //printf("ServerThread! %d\n", verbose);
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
    
    report->packets = 0;
    printf("Server is listening on %s:%d\n", report->group, report->port);
    while (1) {
        old = new;
        old_seq = new_seq;
        char pktbuf[PAYLOAD_SIZE];
        unsigned int addrlen = sizeof(addr);
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
        //printf("Packet\n");
        report->packets++;
        report->packet_batch++;
        memcpy(buffer, pktbuf, 8);
        buffer[9]='\0';
        new_seq = strtol(buffer, NULL, 16);
        pktbuf[nbytes] = '\0';
        (verbose) && printf("%.8s\n", pktbuf);                        
        clock_gettime(CLOCK_REALTIME, &new);
        if (new_seq == 0) {
            printf("Server: New test detected\n");
            report->packet_batch = 1;
            //report->missing = 0;
            //report->miss_events = 0;
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
