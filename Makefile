CC=g++
INCFLAGS= -I $$BOOST_ROOT
BOOST_LIBS= -lboost_program_options
CFLAGS=-Wall -Wextra -std=c++17 -O3 $(INCFLAGS)
SRCDIR=src
OBJDIR=obj
LIBOBJ = $(addprefix $(OBJDIR)/, fastqview.o file_utils.o comparator.o seq_utils.o)
MAINOBJ = $(OBJDIR)/main.o

all: fastq-dupaway

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $< 

fastq-dupaway:  $(LIBOBJ) $(MAINOBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(BOOST_LIBS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf obj/*.o

.phony: clean
