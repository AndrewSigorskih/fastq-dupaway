CC=g++ 
INCFLAGS= -I $$BOOST_ROOT
BOOST_LIBS= -lboost_program_options
CFLAGS=-Wall -Wextra -std=c++17 -O3 $(INCFLAGS)
OBJDIR=obj
LIBOBJ = $(addprefix $(OBJDIR)/, file_utils.o fastq.o)
MAINOBJ = $(OBJDIR)/main.o

all: fastq-dupaway

obj/%.o : src/%.cpp $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $<

fastq-dupaway: $(MAINOBJ) $(LIBOBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(BOOST_LIBS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf obj/*.o