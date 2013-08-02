#----------------------------------------------------------------------------------
#  Make File for synop_tools
#----------------------------------------------------------------------------------

GCC     = gcc
CXX     = g++ 

INCLUDE = -I../ -I../include -I../libtiniconv -I../irrxml

OPTIM		=  -O2 -Wunused  -fexceptions -Wno-deprecated
CXXFLAGS	= $(OPTIM) $(INCLUDE) \
	-DLOCALEDIR=\"/usr/local/share/locale\" \
	-DPKGDATADIR=\"/usr/local/share/data\"
CFLAGS		= $(CXXFLAGS)

# libraries to link with:
LIBS		= -lpthread -ldl -lm -lrt

# ----------------------------------------------------------------------------------

.SUFFIXES: .cxx .o

all: synop_tool

synop_tool.o: synop_tool.cxx
	$(CXX) $(CXXFLAGS) -c $<

synop.o: synop.cxx
	$(CXX) $(CXXFLAGS) -c $<

coordinate.o: ../misc/coordinate.cxx
	$(CXX) $(CXXFLAGS) -c $<

kmlserver.o: ../misc/kmlserver.cxx
	$(CXX) $(CXXFLAGS) -c $<

irrXML.o: ../irrxml/irrXML.cpp
	$(CXX) $(CXXFLAGS) -c $<

re.o: ../misc/re.cxx
	$(CXX) $(CXXFLAGS) -c $<

strutil.o: ../misc/strutil.cxx
	$(CXX) $(CXXFLAGS) -c $<

timeops.o: ../misc/timeops.cxx
	$(CXX) $(CXXFLAGS) -c $<

util.o: ../misc/util.cxx
	$(CXX) $(CXXFLAGS) -c $<

threads.o: ../misc/threads.cxx
	$(CXX) $(CXXFLAGS) -c $<

cmd_debug.o: cmd_debug.cxx
	$(CXX) $(CXXFLAGS) -c $<

nls.o: ../misc/nls.cxx
	$(CXX) $(CXXFLAGS) -c $<

locator.o: ../misc/locator.c
	$(GCC) $(CFLAGS) -c $<

synop_tool: \
	synop_tool.o synop.o coordinate.o kmlserver.o irrXML.o re.o strutil.o \
	nls.o util.o cmd_debug.o threads.o timeops.o locator.o
	$(CXX) \
	synop_tool.o synop.o coordinate.o kmlserver.o irrXML.o re.o strutil.o \
	nls.o util.o cmd_debug.o threads.o timeops.o locator.o \
	-o synop_tool $(LIBS) 
       
clean:
	$(RM)  *.bak *~ *.o synop_tool

#----------------------------------------------------------------------------------
