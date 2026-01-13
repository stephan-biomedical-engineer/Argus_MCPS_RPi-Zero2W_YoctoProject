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
CC  ?= gcc
AR  ?= ar

# ===============================
# INCLUDES
# ===============================
# Se o boost-mqtt5 estiver em um path não padrão, adicione aqui -Ipath/to/mqtt5/include
INC_FLAGS := \
	-Ihal/gpio \
	-Ihal/spi \
	-Ihal/i2c \
	-Idrivers \
	-Idrivers/utl \
	-Iservices \
	-Iserver_layer \
	-Isystem \
	-Iuser_application

# C++17 é obrigatório para Boost.MQTT5
CXXFLAGS := -Wall -O2 -std=c++17 $(INC_FLAGS)
CFLAGS   := -Wall -O2 -std=c99 $(INC_FLAGS)

# ===============================
# Fontes
# ===============================

# 1. HAL
HAL_SRCS := \
	hal/gpio/hal_gpio.cpp \
	hal/spi/hal_spi.cpp \
	hal/i2c/hal_i2c.cpp

# 2. Drivers
DRIVER_CPP_SRCS := drivers/stm32_bridge.cpp
DRIVER_C_SRCS := \
	drivers/cmd.c \
	drivers/utl/utl_io.c \
	drivers/utl/utl_crc16.c

# 3. Services
SERVICE_SRCS := services/infusion_manager.cpp

# 4. App Main
APP_SRCS := system/main.cpp

# Agrupamento
CORE_CPP_SRCS := $(HAL_SRCS) $(DRIVER_CPP_SRCS) $(SERVICE_SRCS)
CORE_C_SRCS   := $(DRIVER_C_SRCS)

# ===============================
# Objetos
# ===============================
CORE_OBJS := $(CORE_CPP_SRCS:%.cpp=$(OBJ_DIR)/%.o) $(CORE_C_SRCS:%.c=$(OBJ_DIR)/%.o)
APP_OBJS  := $(APP_SRCS:%.cpp=$(OBJ_DIR)/%.o)

# ===============================
# Libs externas
# ===============================
# Boost.MQTT5 usa SSL se configurado, mas aqui usamos TCP puro
# Linkamos Boost.System e Boost.JSON e Pthread
LIBS := -lgpiod -lsystemd -lboost_system -lboost_json -lpthread

# ===============================
# Regras
# ===============================

all: $(TARGET)

$(CORE_LIB): $(CORE_OBJS)
	@echo "Arquivando $@"
	$(AR) rcs $@ $^

$(TARGET): $(CORE_LIB) $(APP_OBJS)
	@echo "Linkando $@"
	$(CXX) $(CXXFLAGS) $(APP_OBJS) $(CORE_LIB) -o $@ $(LIBS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compilando C++: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compilando C:   $<"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(CORE_LIB) $(TARGET)

.PHONY: all clean
