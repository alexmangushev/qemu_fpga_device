#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <inttypes.h>

//#define MEM_SIZE 0x6000000
//#define BASE_SIZE 0x6000000

// for socket
int server_port = 2303; // 	Halo: Combat Evolved multiplayer host

//static uint32_t memory[MEM_SIZE/4] = {};

#define printerr(fmt, ...)               \
    do                                   \
{                                        \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    fflush(stderr);                      \
} while (0)


int main(void)
{
    //for /dev/mem part
    int fd;
    void *map_base, *virt_addr;
    uint64_t read_result = -1;
    int access_size = 4;
    unsigned int pagesize = (unsigned)sysconf(_SC_PAGESIZE);
    unsigned int map_size = pagesize;
    unsigned offset;


    // for socket part
    int socket_desc, client_sock, client_size;
    struct sockaddr_in server_addr, client_addr;
    char server_message[22], client_message[22];
    
    // Clean buffers:
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));
    
    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr("172.16.7.11");
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");
    
    while(1) {

        // Listen for clients:2303
        if(listen(socket_desc, 1) < 0){
            printf("Error while listening\n");
            return -1;
        }

        printf("\nListening for incoming connections.....\n");

        // Accept an incoming connection:
        client_size = sizeof(client_addr);
        client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
        
        if (client_sock < 0){
            printf("Can't accept\n");
            return -1;
        }
        printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Receive client's message:
        if (recv(client_sock, client_message, sizeof(client_message), 0) < 0){
            printf("Couldn't receive\n");
            return -1;
        }

        uint32_t addr = 0, data = 0;
        uint16_t wr = 0;

        wr |= client_message[0] << 8;
        wr |= client_message[1];

        for (int i = 3, k = 2; i >= 0; i--, k++)
            addr |= (client_message[k] << (i * 8));

        for (int i = 3, k = 6; i >= 0; i--, k++)
            data |= (client_message[k] << (i * 8));

        printf("Msg from client: %x %x %x\n", wr, addr, data);


        offset = (unsigned int)(addr & (pagesize - 1));
        if (offset + access_size > pagesize)
        {
            // Access straddles page boundary:  add another page:
            map_size += pagesize;
        }
        fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (fd == -1)
        {
            printerr("Error opening /dev/mem (%d) : %s\n", errno, strerror(errno));
            exit(1);
        }
        map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
            fd,
            addr & ~((typeof(addr))pagesize - 1));
        if (map_base == (void *)-1)
        {
            printerr("Error mapping (%d) : %s\n", errno, strerror(errno));
            exit(1);
        }
        virt_addr = map_base + offset;

        // write request
        if (wr) 
        {
            *((volatile uint32_t *)virt_addr) = data;
            //memory[(addr - BASE_SIZE) / 4] = data;   
        }
        else 
        {
            //for (int i = 3, k = 0; i >= 0; i--, k++)
            //    server_message[k] = memory[(addr - BASE_SIZE) / 4] >> (i * 8);

            read_result = *((volatile uint32_t *)virt_addr);
            //printf("%x\n",read_result);
            for (int i = 3, k = 0; i >= 0; i--, k++)
            {
                server_message[k] = read_result >> (i * 8);
                printf("%x\n",server_message[k]);
            }
            
            
            if (send(client_sock, server_message, 4, 0) < 0){
                printf("Can't send\n");
                return -1;
            }
        }

        if (munmap(map_base, map_size) != 0)
        {
            printerr("ERROR munmap (%d) %s\n", errno, strerror(errno));
        }
        close(fd);

        // Closing the socket:
        close(client_sock);
    }
    
    close(socket_desc);
    
    return 0;
}
