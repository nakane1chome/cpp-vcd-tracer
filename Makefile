# A Makefile is a place where your can put those commands you run a lot.
# It's like a README, but you don't need to copy the commands to a console.
# It's like a shell script but you don't need to run it all.
# It's like a json/yaml configuration, but a bit harder to learn.

SRC_DIRS=src test
ALL_SRC=$(shell find ${SRC_DIRS} -name "*.hpp" -or -name "*.cpp"  -or -name "*.ipp")
export CLICOLOR=0

all: test

build : 
	cmake -S . -B build -DENABLE_FUZZING:BOOL=OFF

build_clang  :
	CC=clang CXX=clang++ cmake -S . -B build_clang -DENABLE_FUZZING:BOOL=ON

build_cov :
	cmake -S . -B ./build_cov -DCMAKE_BUILD_TYPE:STRING=Release -DENABLE_COVERAGE:BOOL=ON -DENABLE_FUZZING=OFF

make : build
	cmake --build build

make_clang : build_clang
	cmake --build build_clang

make_cov : build_cov
	cmake --build build_cov

test: make
	cd build; ctest -C Release 

test_cov: make_cov 
	cd build_cov; ctest -C Release 
	gcovr --delete --root ../ --print-summary --xml-pretty --xml coverage.xml .

test_clang: make_clang
	cd build_clang; ctest -C Release 

format:
	clang-format-14 -i ${ALL_SRC}

lint: build_clang
	clang-tidy-14 \
		-p build_clang \
		src/vcd_tracer.cpp

clean :
	rm -rf build build_clang build_cov
