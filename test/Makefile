VAST_KIT=..
INSTALL_DIR := $(VAST_KIT)/bin
CC := $(CC)
CXX := $(CXX)
CFLAGS = -Wall -Wno-missing-braces 
CXXFLAGS = -Wall -g -DDEBUG -O0
VAPP_INCLUDE = -I$(VAST_KIT)/include
VAPP_LIB = -L$(VAST_KIT)/lib -L/opt/vastai/vaststream/lib/  -lvappi -ldl
LDFLAGS =

SRCS := $(wildcard *.c)  
CPPSRCS := $(wildcard *.cpp)
OBJS = $(SRCS:.c=.o) 
CPPOBJS = $(CPPSRCS:.cpp=.o) 
EXEC = test

all: $(EXEC)



# Build rule for debug version
# Debug flags
DEBUG_FLAGS := -g -DDEBUG -O0
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(EXEC)

# Build rule for release version
# Release flags
RELEASE_FLAGS := -O2
release: CFLAGS += $(RELEASE_FLAGS)
release: $(EXEC)

$(EXEC): $(OBJS) $(CPPOBJS)
	@echo $(OBJS) 
	@echo $(CPPOBJS)
	$(CXX) $(CFLAGS) $(VAPP_INCLUDE) -o $@ $^ $(VAPP_LIB) $(PTHREAD_LIB) $(LDFLAGS)
	

%.o: %.c
	$(CC) $(CFLAGS) $(VAPP_INCLUDE) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(VAPP_INCLUDE) -c -o $@ $<	

# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) $(VAPP_INCLUDE) -c -o $@ $<
# Install rule
install: $(EXEC)
	cp $(EXEC) $(INSTALL_DIR) 
clean:
	@echo $(OBJS) 
	@echo $(SRCS) 
	rm -f $(EXEC) $(OBJS) $(CPPOBJS)
