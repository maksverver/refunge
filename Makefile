CFLAGS=-g -O2 -Wall
LDFLAGS=-g
CC=gcc
CXX=g++
CXXFLAGS=$(CFLAGS) -I/usr/include/fltk-1.1
INTERPRETER_OBJ=interpreter.o main.o
DEBUGGER_OBJ=interpreter.o debugger.o
EBUGGER_OBJ=interpreter.o ebugger.o

all: interpreter debugger bf2ref ebugger

interpreter: $(INTERPRETER_OBJ)
	$(CC) $(LDFLAGS) -o interpreter $(INTERPRETER_OBJ)

debugger: $(DEBUGGER_OBJ)
	$(CXX) $(LDFLAGS) -L/usr/lib/fltk-1.1 -lfltk -o debugger $(DEBUGGER_OBJ)

ebugger: $(EBUGGER_OBJ)
	$(CXX) $(LDFLAGS) -L/usr/lib/fltk-1.1 -lfltk -lfltk_gl -o ebugger $(EBUGGER_OBJ)

clean:
	rm -f *.o

distclean: clean
	rm -f interpreter debugger bf2ref
