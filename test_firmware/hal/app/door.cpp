#include "door.hpp"
#include "hal_gpio.hpp"
#include <thread>
#include <atomic>

static HalGpio door_gpio(17, HalGpio::Direction::Input, HalGpio::Edge::Both, false, "/dev/gpiochip0");
static std::atomic<DoorState> g_door_state;
static DoorCallback g_callback = nullptr;

class DoorControllerThread {
    public:
        DoorControllerThread() : _running(true), _thread(&DoorControllerThread::run, this) {}

    ~DoorControllerThread() {
        _running = false;
        if (_thread.joinable())
            _thread.join();
    }

    private:
        std::atomic<bool> _running;
        std::thread _thread;

        void run() {
            while (_running) {
                int ret = door_gpio.wait_for_edge(-1);
                if (ret <= 0)
                    continue;

                DoorState new_state = door_gpio.get() ? DoorState::OPEN : DoorState::CLOSED;
                DoorState old_state = g_door_state.load();
                if (new_state != old_state) {
                    g_door_state.store(new_state);

                    if (g_callback) {
                        g_callback(new_state);
                    }
                }
            }
        }
};

static DoorControllerThread* g_thread = nullptr;

void door_init() {
    g_door_state.store(door_gpio.get() ? DoorState::OPEN : DoorState::CLOSED);
    if (!g_thread) {
        g_thread = new DoorControllerThread();
    }
}

void door_shutdown() {
    if (g_thread) {
        delete g_thread;
        g_thread = nullptr;
    }
}

bool door_is_open() {
    return g_door_state.load() == DoorState::OPEN;
}

void door_register_callback(DoorCallback cb) {
    g_callback = cb;
}
