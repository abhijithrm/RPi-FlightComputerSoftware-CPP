CXX       := g++
CXX_FLAGS := -std=c++11 -ggdb

LDFLAGS  = -pthread

BIN     := bin
SRC     := src
INCLUDE := include

LIBRARY_PATH   := lib
EXTERNAL_LIBRARIES  := -lconfig4cpp  -lprotobuf -lmavsdk -lmavsdk_action -lmavsdk_offboard -lmavsdk_telemetry -lmavsdk_info
EXECUTABLE  := app


all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/*.cpp
	$(CXX) $(CXX_FLAGS) $^ src/ProtoData.pb.cc -o $@ -I$(INCLUDE)  -I/usr/local/include/mavsdk -I/usr/local/include/mavsdk/plugins/action -I/usr/local/include/mavsdk/plugins/telemetry -I/usr/local/include/mavsdk/plugins/offboard  -L$(LIBRARY_PATH) $(EXTERNAL_LIBRARIES) $(LDFLAGS)

clean:
	-rm $(BIN)/*

