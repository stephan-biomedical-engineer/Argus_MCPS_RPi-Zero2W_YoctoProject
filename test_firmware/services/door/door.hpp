#ifndef DOOR_HPP
#define DOOR_HPP

#include <cstdint>

enum class DoorState {
    OPEN,
    CLOSED
};

using DoorCallback = void(*)(DoorState);

void door_init();
void door_shutdown();

bool door_is_open();

void door_register_callback(DoorCallback cb);

#endif
