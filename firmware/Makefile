TARGET = infusion-pump-app
SRCS = boost_server.cpp
LIBS = -lgpiod -lsystemd -lboost_system -lpthread

CXX ?= g++
CXXFLAGS += -Wall -O2 -std=c++17

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS) $(LIBS)

deploy:
	@echo "Parando servico e preparando disco..."
	ssh root@192.168.1.6 "systemctl stop infusion-pump && mount -o remount,rw /"
	@echo "Enviando novo binario..."
	scp $(TARGET) root@192.168.1.6:/usr/bin/
	@echo "Protegendo disco e reiniciando..."
	ssh root@192.168.1.6 "mount -o remount,ro / && systemctl start infusion-pump"
	@echo "Deploy concluido! Acompanhando logs:"
	ssh root@192.168.1.6 "journalctl -u infusion-pump -f"

clean:
	rm -f $(TARGET)