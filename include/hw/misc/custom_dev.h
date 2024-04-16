#ifndef HW_CUSTOM_DEV_H
#define HW_CUSTOM_DEV_H

#include "qom/object.h"

uint64_t data_exchange(uint64_t addr, uint64_t data, char wr);

DeviceState *custom_dev_create(hwaddr);

#endif
