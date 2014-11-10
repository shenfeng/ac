.PHONY: debug release clean

gen: api.thrift
	rm -rf gen-cpp
	thrift -gen cpp api.thrift

debug:
	mkdir -p debug && cd debug && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j

release:
	mkdir -p release && cd release && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j

all: clean gen debug release

clean:
	$(RM) -r release debug gen-cpp
