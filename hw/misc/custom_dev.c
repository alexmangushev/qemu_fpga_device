#include "qemu/osdep.h"
#include "qapi/error.h" // provides error_fatal() handler 
#include "hw/sysbus.h" // provides all sysbus registering func 
#include "hw/misc/custom_dev.h"
#include "hw/irq.h"
#include <sys/socket.h>
#include <arpa/inet.h>

// for socket
int server_port = 2303; // Halo: Combat Evolved multiplayer host

#define SERVER_IP "127.0.0.1"

#define DEV_ADDR 0x46000000

#define TYPE_CUSTOM_DEV "custom_dev"

typedef struct CustomDevState CustomDevState;

DECLARE_INSTANCE_CHECKER(CustomDevState, CUSTOM_DEV, TYPE_CUSTOM_DEV)

struct CustomDevState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    qemu_irq irq;
};

uint64_t data_exchange(uint64_t addr, uint64_t data, char wr)
{
    //if (wr)
    //    printf("\nWRITE: %lx %lx\n", addr, data);
    //else
    //    printf("\nREAD: %lx\n", addr);

    // for socket part
    int socket_desc;
    struct sockaddr_in server_addr;
    char server_message[22], client_message[22];

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Send connection request to server:
    if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        return -1;
    }

    client_message[0] = 0;
    client_message[1] = wr;

    for (int i = 3, k = 2; i >= 0; i--, k++)
        client_message[k] = addr >> (i * 8);
    
    for (int i = 3, k = 6; i >= 0; i--, k++)
        client_message[k] = data >> (i * 8);

    // Send the message to server:
    if(send(socket_desc, client_message, 10, 0) < 0){
        printf("Unable to send message\n");
        return -1;
    }

    if (!wr) {
        // Receive the server's response:
        if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
            printf("Error while receiving server's msg\n");
            return -1;
        }

        uint32_t rdata = 0;
        for (int i = 3, k = 0; i >= 0; i--, k++)
            rdata |= ((uint8_t)server_message[k] << (i * 8));
        return rdata;
    }

    // Close the socket:
    close(socket_desc);

    return 0;
}

static uint64_t custom_dev_read(void *opaque, hwaddr offset, unsigned int size)
{
    //CustomDevState *s = opaque;
    return data_exchange(offset | DEV_ADDR, 0, 0);
}

static void custom_dev_write(void *opaque, hwaddr offset, uint64_t value, unsigned int size)
{
    //CustomDevState *s = opaque;
    data_exchange(offset | DEV_ADDR, value, 1);
}

static const MemoryRegionOps custom_dev_ops = {
    .read = custom_dev_read,
    .write = custom_dev_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void custom_dev_instance_init(Object *obj)
{
    CustomDevState *s = CUSTOM_DEV(obj);

    // allocate memory map region 
    memory_region_init_io(&s->iomem, obj, &custom_dev_ops, s, TYPE_CUSTOM_DEV, 0x2000000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);
}

// create a new type to define the info related to our device
static const TypeInfo custom_dev_info = {
    .name = TYPE_CUSTOM_DEV,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(CustomDevState),
    .instance_init = custom_dev_instance_init,
};

static void custom_dev_register_types(void)
{
    type_register_static(&custom_dev_info);
}

type_init(custom_dev_register_types)

//
// Create the Custom device.
//
DeviceState *custom_dev_create(hwaddr addr)
{
    DeviceState *dev = qdev_new(TYPE_CUSTOM_DEV);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, addr);
    return dev;
}