/*  UDP Packet Broadcasting Daemon

Reads the number of a divert port and a list of destination IP addresses
from the command line. All packets passed through the divert port are
forwarded to the destination addresses by resending the packets with a
modified destination address, with the exception that packets are not sent
back to the host they came from.

Use the IP firewall to send the desired packets to the divert socket.

Copyright (C) 2004 by Maks Verver */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <arpa/inet.h>


/* Global variables */
static char *name;              /* Program name */
static int verbosity;           /* Verbosity; 0 or 1 */
static in_port_t port;          /* Divert port number */
static in_addr_t *destinations; /* Array of destination addresses, terminated
                                   by INADDR_NONE */
static int divert_socket;       /* The divert socket on which packets to be
                                   processed are received and transmitted */
                                   
/* Function prototypes */
static void parse_arguments(int argc, char *argv[]);
static void initialize();
static void process();

/* The application entry point */
int main(int argc, char *argv[])
{
    parse_arguments(argc, argv);
    initialize();
    process();

    return 0;
}


/* Parses the command line arguments */
void parse_arguments(int argc, char *argv[])
{
    char dummy;
    
    /* Set program name */
    name = strrchr(*argv, '/');
    name = name ? name+ 1 : *argv;
    ++argv; --argc;

    /* Check for arguments */
    if(argc < 1)
    {
        fprintf(stdout, "Usage: %s [-v] port destination...\n", name);
        exit(0);
    }

    /* Parse options */
    verbosity = 0;
    if(argc > 0 && strcmp(*argv, "-v") == 0)
    {
        verbosity = 1;
        ++argv; --argc;               
    }
    
    /* Parse divert port number */
    if(argc == 0)
    {
        fprintf(stderr, "%s: No divert port number specified\n", name);
        exit(0);
    }
    else
    if(sscanf(*argv, "%hu%c", &port, &dummy) != 1)
    {
        fprintf( stderr, "%s: %s: Invalid divert port number\n",
                 name, *argv );
        exit(1);
    }
    port = htons(port);
    ++argv; --argc;
    
    /* Parse destinations addresses */
    if(argc == 0)
    {
        fprintf(stderr, "%s: No destination addresses specified\n", name);
        exit(0);
    }
    else
    {
        int n;
        
        destinations = (in_addr_t*)malloc(sizeof(in_addr_t)*(argc + 1));
        for(n = 0; n < argc; ++n)
        {
            destinations[n] = inet_addr(argv[n]);
            if(destinations[n] == INADDR_NONE)
            {
                fprintf( stderr, "%s: %s: Invalid destination address\n",
                         name, argv[n] );
                exit(1);
            }
        }
        destinations[n] = INADDR_NONE;
    }
}

/* Initalizes the divert socket */
void initialize()
{
    struct sockaddr_in socket_address;
    
    /* Create divert socket */
    if(verbosity > 0)
    {
        printf("Creating divert socket...\n");
    }
    divert_socket = socket(PF_INET, SOCK_RAW, IPPROTO_DIVERT);
    if(divert_socket < 0)
    {
        perror(name);            
        exit(1);
    }
    
    /* Bind divert socket */
    if(verbosity > 0)
    {
        printf("Binding divert socket to port %hu...\n", ntohs(port));
    }
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = 0;
    socket_address.sin_port = port;
    if(bind( divert_socket, (struct sockaddr *)&socket_address,
             sizeof(socket_address) ) < 0)
    {
        perror(name);            
        exit(1);
    }
}

/* Processes incoming packets by transmitting them to their destinations */
void process()
{
    union {
        struct udpiphdr header;
        char buffer[8192];
    } packet;
    ssize_t packet_size;
    struct sockaddr_in packet_address;
    socklen_t packet_address_size;

    if(verbosity > 0)
    {
        printf("Waiting for incoming packets...\n");
    }

    while(1)
    {
        /* Receive a packet */
        packet_address_size = sizeof(packet_address);
        packet_size = recvfrom(
            divert_socket, packet.buffer, sizeof(packet.buffer), 0,
            (struct sockaddr*)&packet_address, &packet_address_size );
            
        if(packet_size < 0)
        {
            perror(name);
        }
        else
        if(packet_size >= sizeof(packet.header) && packet.header.ui_i.ih_pr == IPPROTO_UDP)
        {
            ssize_t sent_size;
            in_addr_t *addr;
            
            if(verbosity > 0)
            {
                printf( "\nPacket of size %d received!\n", packet_size );
                printf( "Source address: %s\n",
                        inet_ntoa(packet.header.ui_i.ih_src) );
                printf( "Destination address: %s\n",
                        inet_ntoa(packet.header.ui_i.ih_dst) );
            }

            /* Forward to all destination addresses. */            
            for(addr = destinations; *addr != INADDR_NONE; ++addr)
            {
                if(*addr == packet.header.ui_i.ih_src.s_addr)
                    continue;

                packet.header.ui_i.ih_dst.s_addr = *addr;

                if(verbosity > 0)
                {
                    printf( "Forwarding to: %s\n",
                            inet_ntoa(packet.header.ui_i.ih_dst) );
                }

                /* Send out modified packet */
                sent_size = send(divert_socket, packet.buffer, packet_size, 0);
                /*
                sent_size = sendto(
                    divert_socket, packet.buffer, packet_size, 0,
                    (struct sockaddr*)&packet_address, packet_address_size );
                */
                
                if(sent_size < 0)
                {
                    perror(name);
                }
                else
                if(sent_size < packet_size)
                {
                    fprintf(
                        stderr, "%s: Packet of size %d truncated to %d bytes\n",
                        name, packet_size, sent_size );
                }
            }
            
        }
    }
}

/* End of file */
