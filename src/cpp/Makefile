CC = g++
CFLAGS = -fpic -g -I.
LFLAGS = -shared

APPNAME = iniparser.so

LIBS =
OBJ = iniparser.o
DEP = iniparser.h

%.o: %.cpp $(DEP)
	$(CC) $(CFLAGS) -c $<

$(APPNAME): $(OBJ)
	$(CC) -o $@ $^ $(LFLAGS) $(LIBS)


.PHONY: clean

clean:
	rm -f *.o *~
veryclean:
	rm -f *.0 *~ $(APPNAME)


