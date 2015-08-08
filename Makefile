LIBS = -pthread
FLAGS = -g -Wall -Werror -std=c++11

src = $(wildcard *.cpp)
objects = $(patsubst %.cpp, %.o, $(src))

dst = ADNS

$(dst): $(objects)
	g++  $(objects) $(LIBS) $(FLAGS) -o $@


$(objects):%.o : %.cpp 
	g++ -c $(FLAGS)  $<  -o $@


.PHONY:clean
clean:
	-rm -f *.o
	-rm -f $(dst) 

