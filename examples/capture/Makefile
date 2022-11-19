# Compiler:
CC=g++

OUTFOLDER = bin/

# Used standart libs:
STANDART_LIBS = -lpthread -lstdc++fs

# Compiler flags:
CCFLAGS = -std=c++14 
LDFLAGS = -fdiagnostics-color=always

# Platform-dependent variables:
ifeq ($(OS), Windows_NT)
OTHER_LIBS = -lws2_32 -lwsock32 -lmswsock -lgdi32
EXECUTABLE = netshot.exe
else
OTHER_LIBS = -lX11
EXECUTABLE = netshot.out
endif

# Source files:
SOURCES= main.cpp \
 server.cpp \
 client.cpp \
 network/srfc_request.cpp \
 network/srfc_response.cpp \
 network/srfc_connection.cpp \
 network/srfc_listener.cpp \
 network/unix/srfc_connection_unix.cpp \
 network/unix/srfc_listener_unix.cpp \
 network/win32/srfc_connection_win32.cpp \
 network/win32/srfc_listener_win32.cpp \
 screencap/screencap.cpp \
 screencap/unix/screencap_unix.cpp \
 screencap/win32/screencap_win32.cpp

OBJECTS=$(SOURCES:.cpp=.o)

all: pre $(EXECUTABLE) clean

# compile
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(foreach binObject, $(notdir $(foreach object, $(OBJECTS), $(object))), $(OUTFOLDER)$(binObject)) -o $(OUTFOLDER)$@ $(STANDART_LIBS) $(OTHER_LIBS)

.cpp.o:
	$(CC) $(CCFLAGS) -c $< -o $(OUTFOLDER)$(@F)

pre:
	rm -r -f $(OUTFOLDER) && mkdir $(OUTFOLDER)

clean: 
	rm -f $(foreach binObject, $(notdir $(foreach object, $(OBJECTS), $(object))), $(OUTFOLDER)$(binObject))