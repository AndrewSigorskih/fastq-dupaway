CC=g++ 
INCFLAGS= -I $$BOOST_ROOT
BOOST_LIBS= -lboost_program_options
CFLAGS=-Wall -Wextra -std=c++17 -O3 $(INCFLAGS)
LIBOBJ = obj/file_utils.o obj/fastq.o
MAINOBJ = obj/main.o

obj/%.o : src/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

fastq-dupaway: $(MAINOBJ) $(LIBOBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(BOOST_LIBS)

all: fastq-dupaway

clean:
	rm -rf obj/*.o