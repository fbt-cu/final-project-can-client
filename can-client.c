/******************************************************
# can-socket application
# Author: Fernando Becerra Tanaka <fernando.becerratanaka@colorado.edu>
# Based on the work of Induja Narayanan <Induja.Narayanan@colorado.edu>
******************************************************/
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

#define CLIENT_BUFFER_LEN 1024

bool exit_main_loop = false;

typedef struct
{
    int server_fd;
    struct sockaddr_storage socket_addr;
} ThreadArgs;

typedef struct thread_Node
{
    pthread_t thread_id;
    int client_fd;
    int alive;
    SLIST_ENTRY(thread_Node) entry;
}thread_Node;

// Define the head of the list
SLIST_HEAD(ThreadList, thread_Node) head = SLIST_HEAD_INITIALIZER(head);

// Global mutex for synchronizing access to print
///TODO: this mutex might not be really needed, for now is for debugging purposes
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
// Global mutex for synchronizing access to the file
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
// Global mutex for synchronizing access to the thread nodes
pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_thread_node(pthread_t thread_id, int client_fd)
{
   thread_Node* new_thread_node = malloc(sizeof(thread_Node));
   if(!new_thread_node)
   {
    syslog(LOG_ERR,"Failed to allocate memory for thread node");
    return;
   }
   new_thread_node->thread_id = thread_id;
   new_thread_node->client_fd = client_fd;
   new_thread_node->alive = 1;
   pthread_mutex_lock(&thread_list_mutex);
   syslog(LOG_INFO,"Inserting thread node");
   SLIST_INSERT_HEAD(&head,new_thread_node,entry);
   pthread_mutex_unlock(&thread_list_mutex);

}

void wait_for_all_threads_to_join()
{
    thread_Node* current_thread_node = SLIST_FIRST(&head);
    thread_Node* next_thread_node;
    pthread_mutex_lock(&thread_list_mutex);
    while((current_thread_node != NULL))
    {
        next_thread_node = SLIST_NEXT(current_thread_node,entry);
        pthread_kill(current_thread_node->thread_id, SIGINT);
        if(pthread_join(current_thread_node->thread_id,NULL)==0)
        {
            syslog(LOG_INFO,"Removing the thread node");
            SLIST_REMOVE(&head,current_thread_node,thread_Node,entry);
            free(current_thread_node);
        }
        else
        {
            syslog(LOG_INFO,"Thread %ld was not able to join: %s",current_thread_node->thread_id, strerror(errno));
        }
        current_thread_node = next_thread_node;
    }
  
    pthread_mutex_unlock(&thread_list_mutex);
}

void signal_handler(int signal)
{
    if (signal == SIGINT)
    {
        syslog(LOG_INFO, "Caught SIGINT (Ctrl+C), exiting gracefully\n");
    }
    else if (signal == SIGTERM)
    {
        syslog(LOG_INFO, "Caught SIGTERM, exiting gracefully\n");
    }
    // Set the global variable so the main server exits gracefully
    exit_main_loop = true;
}

void initialize_sigaction()
{
    struct sigaction sighandle;
    // Initialize sigaction
    sighandle.sa_handler = signal_handler;
    sigemptyset(&sighandle.sa_mask); // Initialize the signal set to empty
    sighandle.sa_flags = 0;          // No special flags

    // Catch SIGINT
    if (sigaction(SIGINT, &sighandle, NULL) == -1)
    {
        syslog(LOG_ERR, "Error setting up signal handler SIGINT: %s \n", strerror(errno));
    }

    // Catch SIGTERM
    if (sigaction(SIGTERM, &sighandle, NULL) == -1)
    {
        syslog(LOG_ERR, "Error setting up signal handler SIGINT: %s \n", strerror(errno));
    }
}

void *thread_function(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    char client_ip[INET_ADDRSTRLEN];
    struct can_frame frame;
    int nbytes;
    pthread_t self;

    self = pthread_self();

    memset(&frame, 0, sizeof(struct can_frame));
    // Convert binary IP address from binary to human readable format

    if (threadArgs->socket_addr.ss_family == AF_INET)
    { // Check if the address is IPv4
        struct sockaddr_in *addr_in = (struct sockaddr_in *)&threadArgs->socket_addr;
        inet_ntop(AF_INET, &(addr_in->sin_addr), client_ip, sizeof(client_ip));
    }
    else if (threadArgs->socket_addr.ss_family == AF_INET6)
    { // Check if the address is IPv6
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&threadArgs->socket_addr;
        inet_ntop(AF_INET6, &(addr_in6->sin6_addr), client_ip, sizeof(client_ip));
    }

    // Log the client ip
    syslog(LOG_INFO, "Created connection to %s", client_ip);

    // start receiving command from connection
    while(!exit_main_loop)
    {

    }

    if (close(threadArgs->server_fd) == 0)
    {
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }
    else
    {
        syslog(LOG_ERR, "Closing of connection from %s failed", client_ip);
    }

    // Exit from the thread
    return 0;
}

int main(int argc, char **argv)
{
    struct addrinfo inputs, *server_info;
    int socket_fd;
    struct sockaddr_storage server_addr;
    socklen_t server_addr_size;
    int status;
    int yes = 1;
    int ret;

    // Open a system logger connection for aesdsocket utility
    openlog("can-client", LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);

    /*Line  was partly referred from https://beej.us/guide/bgnet/html/#socket */
    memset(&inputs, 0, sizeof(inputs));
    inputs.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    inputs.ai_socktype = SOCK_STREAM; // TCP stream sockets
    // inputs.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if (argc != 2) 
    {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    // Get address info
    if ((status = getaddrinfo(argv[1], "9000", &inputs, &server_info)) != 0)
    {
        syslog(LOG_ERR, "Error occurred while getting the address info: %s \n", gai_strerror(status));
        closelog();
        exit(1);
    }

    // Open a stream socket
    socket_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (socket_fd == -1)
    {
        syslog(LOG_ERR, "Error occurred while creating a socket: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        closelog();
        exit(1);
    }

    // Set socket options
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        syslog(LOG_ERR, "Error occurred while setting a socket option: %s \n", strerror(errno));
        freeaddrinfo(server_info);
        closelog();
        exit(1);
    }

    initialize_sigaction();
    server_addr_size = sizeof(server_addr);

    // Main server loop
    while (!exit_main_loop)
    {
        getchar();

        ret = connect(socket_fd, server_info->ai_addr, server_info->ai_addrlen);
        if (ret == -1)
        {
            syslog(LOG_ERR, "Error occurred during connect operation: %s \n", strerror(errno));
            continue;
        }

        pthread_t threadId;
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (args == NULL)
        {
            syslog(LOG_ERR, "Failed to allocate memory for thread arguments");
            close(socket_fd);
            continue;
        }

        args->server_fd = socket_fd;
        args->socket_addr = server_addr;

        syslog(LOG_INFO, "Creating a new thread");
        int err = pthread_create(&threadId, NULL, thread_function, (void *)args);
        if (err != 0)
        {
            syslog(LOG_ERR, "Error creating thread: %s", strerror(err));
            close(socket_fd);
            free(args);
            continue;
        }

        add_thread_node(threadId, socket_fd);
    }

    // Clean up before exiting
    syslog(LOG_ERR, "Waiting for active threads to join");
    wait_for_all_threads_to_join();

    freeaddrinfo(server_info);
    closelog();
}