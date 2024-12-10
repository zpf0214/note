CMAKE_COMPILER_OPTS=
CMAKE_TESTS_OPTS=-DENABLE_TESTS=on 
CMAKE_DTESTS_OPTS=${CMAKE_COMPILER_OPTS} -DCMAKE_BUILD_TYPE=debug ${CMAKE_TESTS_OPTS}
CMAKE_OTESTS_OPTS=${CMAKE_COMPILER_OPTS} -DCMAKE_BUILD_TYPE=optim ${CMAKE_TESTS_OPTS}
CMAKE_MTESTS_OPTS=${CMAKE_COMPILER_OPTS} -DCMAKE_BUILD_TYPE=debug ${CMAKE_TESTS_OPTS} -DENABLE_MEMTESTS=ON 
CMAKE_QTESTS_OPTS=${CMAKE_COMPILER_OPTS} -DCMAKE_BUILD_TYPE=debug ${CMAKE_TESTS_OPTS} -DENABLE_QEMUTESTS=ON 
CMAKE_FTESTS_OPTS=${CMAKE_COMPILER_OPTS} -DCMAKE_BUILD_TYPE=optim ${CMAKE_TESTS_OPTS} -DENABLE_FPGATESTS=ON 

# for build failed 11
all: build
	make -C build

install: build
	make -C build install

clean: build
	make -C build clean

distclean:
	rm -rf build builddtests buildotests buildmtests buildqtests buildftests; true

test: otest dtest mtest

otest: buildotests
	make -j $(nproc) -C buildotests VERBOSE=1
	make -j $(nproc) -C buildotests test VERBOSE=1


dtest: builddtests
	make -j $(nproc) -C builddtests VERBOSE=1
	make -j $(nproc) -C builddtests test VERBOSE=1


mtest: buildmtests
	make -j $(nproc) -C buildmtests VERBOSE=1
	make -j $(nproc) -C buildmtests test VERBOSE=1

qtest: buildqtests
	make -j $(nproc) -C buildqtests VERBOSE=1
	make -j $(nproc) -C buildqtests test VERBOSE=1


ftest: buildftests
	make -j $(nproc) -C buildftests test VERBOSE=1


build: 
	mkdir build; cd build; cmake ../src $(CMAKE_MUL_OPTS) ; cd ..

builddtests:
	rm -rf $@; true; mkdir $@; 
	cd $@; cmake ../src ${CMAKE_DTESTS_OPTS} $(CMAKE_MUL_OPTS); 
	cd ..

buildotests:
	rm -rf $@; true; mkdir $@; 
	cd $@; cmake ../src ${CMAKE_OTESTS_OPTS} ${CMAKE_MUL_OPTS}; 
	cd ..

buildmtests:
	rm -rf $@; true; mkdir $@; 
	cd $@; cmake ../src ${CMAKE_MTESTS_OPTS} ${CMAKE_MUL_OPTS}; 
	cd ..


buildqtests:
	rm -rf $@; true; mkdir $@; 
	cd $@; cmake ../src ${CMAKE_QTESTS_OPTS} ${CMAKE_MUL_OPTS}; 
	cd ..

buildftests:
	rm -rf $@; true; mkdir $@; 
	cd $@; 	cmake ../src ${CMAKE_FTESTS_OPTS} ${CMAKE_MUL_OPTS}; make -j $(nproc)  VERBOSE=1
	cd ..


alltests:
	make distclean && make test CMAKE_COMPILER_OPTS="-DCMAKE_CXX_COMPILER=clang++-libc++ -DCMAKE_C_COMPILER=clang"
	make distclean && make test CMAKE_COMPILER_OPTS="-DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang"
	make distclean && make test CMAKE_COMPILER_OPTS="-DCMAKE_CXX_COMPILER=g++-9 -DCMAKE_C_COMPILER=gcc-9"
	make distclean && make test CMAKE_COMPILER_OPTS="-DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_C_COMPILER=gcc-8"
	make distclean && make test CMAKE_COMPILER_OPTS="-DCMAKE_CXX_COMPILER=g++-7 -DCMAKE_C_COMPILER=gcc-7"

