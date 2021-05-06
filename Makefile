CC = g++
CFLAGS = -Wall -O2
BUILD_DIR = build
SRC_DIR = src
LIBS = -lpthread

build: pickles.o mongoose.o RESTserver.o ThreadPool.o
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIBS) -o $(BUILD_DIR)/pickles $(BUILD_DIR)/pickles.o $(BUILD_DIR)/mongoose.o $(BUILD_DIR)/RESTserver.o $(BUILD_DIR)/ThreadPool.o

pickles.o: $(SRC_DIR)/pickles.cpp
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIBS) -c -o $(BUILD_DIR)/pickles.o $(SRC_DIR)/pickles.cpp

mongoose.o: $(SRC_DIR)/RESTserver/mongoose.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIBS) -c -o $(BUILD_DIR)/mongoose.o $(SRC_DIR)/RESTserver/mongoose.c

RESTserver.o: $(SRC_DIR)/RESTserver/RESTserver.cpp
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIBS) -c -o $(BUILD_DIR)/RESTserver.o $(SRC_DIR)/RESTserver/RESTserver.cpp

ThreadPool.o: $(SRC_DIR)/ThreadPool/ThreadPool.cpp
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIBS) -c -o $(BUILD_DIR)/ThreadPool.o $(SRC_DIR)/ThreadPool/ThreadPool.cpp

clean:
	rm -f $(BUILD_DIR)/pickles.o $(BUILD_DIR)/mongoose.o $(BUILD_DIR)/RESTserver.o $(BUILD_DIR)/ThreadPool.o $(BUILD_DIR)/pickles
	rmdir $(BUILD_DIR)
