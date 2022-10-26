all : SmartRouter

CCC = g++
# CCC = clang++-10
CCFLAGS = -O3 -Wall -std=c++14
# CCFLAGS = -O3 -Wall -std=c++2a
# CCFLAGS = -Wall -std=c++2a -g
TARGETDIR=.
SRCDIR = ../src/
HGSDIR = ../src/hgs/
SMARTDIR = ../src/smart/

OBJS2 = \
        $(TARGETDIR)/Genetic.o \
        $(TARGETDIR)/Individual.o \
        $(TARGETDIR)/LocalSearch.o \
        $(TARGETDIR)/Params.o \
        $(TARGETDIR)/Population.o \
        $(TARGETDIR)/Split.o \
        $(TARGETDIR)/Configuration.o \
        $(TARGETDIR)/EAX.o \
        $(TARGETDIR)/Flag.o \
        $(TARGETDIR)/Goal.o \
		$(TARGETDIR)/Problem.o \
		$(TARGETDIR)/Solver.o \
        $(TARGETDIR)/main.o \

$(TARGETDIR)/SmartRouter: $(OBJS2)
	$(CCC) $(CCFLAGS) -o $(TARGETDIR)/SmartRouter $(OBJS2)
	
$(TARGETDIR)/Genetic.o: $(HGSDIR)Genetic.h $(HGSDIR)Genetic.cpp
	$(CCC) $(CCFLAGS) -c $(HGSDIR)Genetic.cpp -o $(TARGETDIR)/Genetic.o

$(TARGETDIR)/Individual.o: $(HGSDIR)Individual.h $(HGSDIR)Individual.cpp
	$(CCC) $(CCFLAGS) -c $(HGSDIR)Individual.cpp -o $(TARGETDIR)/Individual.o

$(TARGETDIR)/LocalSearch.o: $(HGSDIR)LocalSearch.h $(HGSDIR)LocalSearch.cpp
	$(CCC) $(CCFLAGS) -c $(HGSDIR)LocalSearch.cpp -o $(TARGETDIR)/LocalSearch.o

$(TARGETDIR)/Params.o: $(HGSDIR)Params.h $(HGSDIR)Params.cpp
	$(CCC) $(CCFLAGS) -c $(HGSDIR)Params.cpp -o $(TARGETDIR)/Params.o

$(TARGETDIR)/Population.o: $(HGSDIR)Population.h $(HGSDIR)Population.cpp
	$(CCC) $(CCFLAGS) -c $(HGSDIR)Population.cpp -o $(TARGETDIR)/Population.o

$(TARGETDIR)/Split.o: $(HGSDIR)Split.h $(HGSDIR)Split.cpp
	$(CCC) $(CCFLAGS) -c $(HGSDIR)Split.cpp -o $(TARGETDIR)/Split.o

$(TARGETDIR)/Configuration.o: $(SMARTDIR)Configuration.cpp
	$(CCC) $(CCFLAGS) -c $(SMARTDIR)Configuration.cpp -o $(TARGETDIR)/Configuration.o

$(TARGETDIR)/EAX.o: $(SMARTDIR)EAX.cpp
	$(CCC) $(CCFLAGS) -c $(SMARTDIR)EAX.cpp -o $(TARGETDIR)/EAX.o

$(TARGETDIR)/Flag.o: $(SMARTDIR)Flag.cpp
	$(CCC) $(CCFLAGS) -c $(SMARTDIR)Flag.cpp -o $(TARGETDIR)/Flag.o

$(TARGETDIR)/Goal.o: $(SMARTDIR)Goal.cpp
	$(CCC) $(CCFLAGS) -c $(SMARTDIR)Goal.cpp -o $(TARGETDIR)/Goal.o

$(TARGETDIR)/Problem.o: $(SMARTDIR)Problem.cpp
	$(CCC) $(CCFLAGS) -c $(SMARTDIR)Problem.cpp -o $(TARGETDIR)/Problem.o

$(TARGETDIR)/Solver.o: $(SMARTDIR)Solver.cpp
	$(CCC) $(CCFLAGS) -c $(SMARTDIR)Solver.cpp -o $(TARGETDIR)/Solver.o

$(TARGETDIR)/main.o: $(SRCDIR)main.cpp
	$(CCC) $(CCFLAGS) -c $(SRCDIR)main.cpp -o $(TARGETDIR)/main.o

test: SmartRouter
	./SmartRouter ../../instances/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt test.sol -seed 1

clean:
	$(RM) \
    $(TARGETDIR)/main.o \
    $(TARGETDIR)/Genetic.o \
    $(TARGETDIR)/Individual.o \
    $(TARGETDIR)/LocalSearch.o \
    $(TARGETDIR)/Params.o \
    $(TARGETDIR)/Population.o \
    $(TARGETDIR)/Split.o \
	$(TARGETDIR)/Configuration.o \
	$(TARGETDIR)/EAX.o \
	$(TARGETDIR)/Flag.o \
	$(TARGETDIR)/Goal.o \
	$(TARGETDIR)/Problem.o \
	$(TARGETDIR)/Solver.o \