all: hw3
hw3: proxyserver.cpp
	g++ proxyserver.cpp -o proxyserver -pthread

clean:
	rm -rf proxyserver 

