objects=w-w w-w_nest

CPPFLAGS=-lpthread

all: $(objects)

$(objects): % : %.cpp
	g++ $(CPPFLAGS) $< -o $@

clean:
	rm -rf for w-w w-w_nest
