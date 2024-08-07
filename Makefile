CC=g++
INCFLAGS= -I $${BOOST_ROOT}/include
BOOST_LIBS= -L$${BOOST_ROOT}/lib -lboost_program_options -lboost_iostreams
CFLAGS=-Wall -Wextra -std=c++17 -O3 $(INCFLAGS)
SRCDIR=src
OBJDIR=obj
LIBOBJ = $(addprefix $(OBJDIR)/, fastaview.o fastqview.o file_utils.o seq_utils.o comparator.o hash_dup_remover.o)
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
