# ===============================
# Projeto
# ===============================
TARGET := infusion-pump-app
CORE_LIB := libinfusion_core.a

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

# ===============================
# Toolchain
# ===============================
CXX ?= g++
AR  ?= ar

CXXFLAGS := -Wall -O2 -std=c++17
CXXFLAGS += \
    -Ihal/gpio \
    -Ihal/pwm \
    -Ihal/i2c \
    -Ihal/adc \
    -Idrivers/adc \
    -Idrivers/motor \
    -Idrivers/encoder \
    -Idrivers/ir_led \
    -Iservices/door \
    -Iservices/alarm \
    -Iservices/bubble_detector \
    -Isystem

# ===============================
# Fontes
# ===============================

# HAL
HAL_SRCS := \
	hal/gpio/hal_gpio.cpp \
	hal/pwm/hal_pwm.cpp \
	hal/i2c/hal_i2c.cpp \
	hal/adc/hal_adc.cpp

# Drivers
DRIVER_SRCS := \
	drivers/adc/adc.cpp \
	drivers/motor/motor_pwm.cpp \
	drivers/encoder/encoder_position.cpp \
	drivers/ir_led/ir_led_pwm.cpp

# Services
SERVICE_SRCS := \
	services/door/door.cpp \
	services/alarm/alarm.cpp \
	services/bubble_detector/bubble_detector.cpp

CORE_SRCS := $(HAL_SRCS) $(DRIVER_SRCS) $(SERVICE_SRCS)

# System / App
APP_SRCS := \
	system/init_system.cpp 

# ===============================
# Objetos
# ===============================
CORE_OBJS := $(CORE_SRCS:%.cpp=$(OBJ_DIR)/%.o)
APP_OBJS  := $(APP_SRCS:%.cpp=$(OBJ_DIR)/%.o)

# ===============================
# Libs externas
# ===============================
LIBS := -lgpiod -lsystemd -lboost_system -lpthread

# ===============================
# Regras
# ===============================

all: $(TARGET)

# ---- Biblioteca core ----
$(CORE_LIB): $(CORE_OBJS)
	@echo "Arquivando $@"
	$(AR) rcs $@ $^

# ---- Binário final ----
$(TARGET): $(CORE_LIB) $(APP_OBJS)
	@echo "Linkando $@"
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# ---- Regra genérica ----
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(CORE_LIB) $(TARGET)

.PHONY: all clean deploy
