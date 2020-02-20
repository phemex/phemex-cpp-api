VERSION = 0.1.0
##----------------------------------------------------------
A=@
CC    = gcc
CXX   = g++
AR    = ar
DB2   = db2
RM    = rm
ECHO  = @echo
MV    = mv
MAKE  = make
CD    = cd
CP    = cp
MKDIR = mkdir
FIND  = find
XARGS = xargs
STRIP = strip

##-----------install path-----------------------
INSTALL_PATH    = ./install
BOOST_LIB_PATH 	= /usr/local/lib
PROTOC			= protoc
PROTOBUF_HDR    = /usr/include/

##-----------OS/COMPILE SPECIAL FLAGS------------
RELEASE_REVISON=$(VERSION).$(GIT_REVISION)
CXXFLAGS = -O3 -Wall -std=c++17 -DBOOST_LOG_DYN_LINK -DBOOST_ASIO_HAS_STD_CHRONO -DBOOST_COROUTINES_NO_DEPRECATION_WARNING -Wunused-variable -Wno-overloaded-virtual -Wno-deprecated-declarations -fdata-sections -ffunction-sections -DNDEBUG

LDFLAGS = 


##----------------------------------------------------------
INCLUDES  = /usr/local/include \
            /usr/include

DYNAMIC_LINKINGS = -lz

##----------------------------------------------------------
SRCEXTS = .c .cc .cpp

SRC_ROOT     	= .

COMMON_DIRS = $(SRC_ROOT)/common

COMMON_INCLUDES	+= $(SRC_ROOT)/src \
			$(SRC_ROOT)/include

INCLUDES += $(COMMON_INCLUDES)
SOURCES_DIR  += $(COMMON_DIRS)

##----------------------------------------------------------
TARGET 		= phemex-cpp-api

BUILD_DIR 	 = $(SRC_ROOT)/build/$(TARGET)

SOURCES_DIR	+= $(SRC_ROOT)

INCLUDES    += 	.
LIB_PATHS   +=  /usr/local/lib /usr/lib

DYNAMIC_LINKINGS += -lboost_program_options -lboost_system -lboost_log -lboost_log_setup -lboost_thread -lboost_coroutine -lboost_date_time -lboost_filesystem -lboost_regex -lboost_context -lpthread -lssl -lcrypto

##----------------------------------------------------------
.PHONY: all
all: $(TARGET)

test:
	echo $(OBJS)

.PHONY: clean
clean:
	$(FIND) $(BUILD_DIR) -name "*.o" -o -name "*.d" -o -name "*~" | $(XARGS) $(RM) -f
	$(RM) -f $(TARGET)

##----------------------------------------------------------
SOURCES = $(foreach d,$(SOURCES_DIR),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
ABSPATH_OBJS = $(addsuffix .o,$(basename $(SOURCES)))
OBJS = $(patsubst $(SRC_ROOT)/%.o,$(BUILD_DIR)/%.o,$(ABSPATH_OBJS))

DEPS = $(OBJS:.o=.d)

OBJDIRS  = $(sort $(dir $(OBJS)))
__dummy := $(shell mkdir -p $(OBJDIRS))

LIBPATHS  += $(addprefix -L, $(LIB_PATHS))
INCPATHS  += $(addprefix -I, $(INCLUDES))

-include $(DEPS)

$(BUILD_DIR)/%.o:$(SRC_ROOT)/%.cpp
	$(A)$(ECHO) "Compiling [cpp] file:[$@] ..."
	$(CXX) $(CXXFLAGS) $(INCPATHS) -MD -MF $(BUILD_DIR)/$*.d -c $< -o $@

##----------------------------------------------------------

$(TARGET): $(OBJS)
	$(ECHO) "Linking   [bin] file:[$@] ..."
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBPATHS) -o $@ $(DYNAMIC_LINKINGS)
	#$(STRIP) --strip-unneeded $@

