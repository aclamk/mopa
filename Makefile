all: test lib

lib: $(MOPA_LIB)

MOPA_LIB=obj/mopa.a

CXXFLAGS=-fPIC -g -O0

MOPA_LIB_SOURCES= \
	src/commontypes.cpp \
	src/crc.cpp \
	src/descriptors.cpp \
	src/io.cpp \
	src/merger.cpp \
	src/sec2ts.cpp

HEADERS= \
		inc/commontypes.h \
		inc/descriptors.h \
		inc/io.h \
		inc/merger.h \
		inc/sec2ts.h

MOPA_LIB_OBJS=$(patsubst src/%.cpp,obj/%.o,$(MOPA_LIB_SOURCES))

all: lib test

clean:
	rm -f test
	rm -f obj/test.o
	rm -f $(MOPA_LIB_OBJS)
	rm -f $(MOPA_LIB)

obj/%.o: src/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I . -c -o $@ $<

obj/test.o: tests/test.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I . -c -o $@ $<

$(MOPA_LIB): $(MOPA_LIB_OBJS)
	ar cr $(MOPA_LIB) $(MOPA_LIB_OBJS)

test: obj/test.o $(MOPA_LIB)
	$(CXX) -o $@ $^


.PHONY: all clean
