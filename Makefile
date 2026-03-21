SRC_DIRS=src test
ALL_SRC=$(shell find ${SRC_DIRS} -name "*.hpp" -or -name "*.cpp"  -or -name "*.ipp")
export CLICOLOR=0

.PHONY: all
all: test

.PHONY: build
build :
	cmake -S . -B build -DENABLE_FUZZING:BOOL=OFF

.PHONY: build_clang
build_clang  :
	CC=clang CXX=clang++ cmake -S . -B build_clang -DENABLE_FUZZING:BOOL=ON

.PHONY: build_cov
build_cov :
	cmake -S . -B ./build_cov -DCMAKE_BUILD_TYPE:STRING=Release -DENABLE_COVERAGE:BOOL=ON -DENABLE_FUZZING=OFF

.PHONY: make
make : build
	cmake --build build

.PHONY: make_clang
make_clang : build_clang
	cmake --build build_clang

.PHONY: make_cov
make_cov : build_cov
	cmake --build build_cov

.PHONY: test
test: make
	cd build; ctest -C Release

.PHONY: test_cov
test_cov: make_cov
	cd build_cov; ctest -C Release
	gcovr --delete --root ../ --print-summary --xml-pretty --xml coverage.xml .

.PHONY: test_clang
test_clang: make_clang
	cd build_clang; ctest -C Release

.PHONY: format
format:
	clang-format-20 -i ${ALL_SRC}

.PHONY: lint
lint: build_clang
	clang-tidy-14 \
		-p build_clang \
		src/vcd_tracer.cpp

.PHONY: clean
clean :
	rm -rf build build_clang build_cov
