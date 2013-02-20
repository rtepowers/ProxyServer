all: hw3
hw3: proxyserver.cpp
	g++ proxyserver.cpp -o proxyServer -pthread

clean:
	rm -rf proxyServer 

