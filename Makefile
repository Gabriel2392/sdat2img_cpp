CXX = clang++
CXXFLAGS = -Wall -Wextra -Wconversion -Werror -O3

sdat2img: sdat2img.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f sdat2img
