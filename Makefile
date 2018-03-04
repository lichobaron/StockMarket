all: StockMarket broker clean

broker: Broker.o
	gcc -o broker Utils.o Broker.o
StockMarket: StockMarket.o
	gcc -o StockMarket Utils.o StockMarket.o
broker.o: Broker.c StockMarket.h
	gcc -c Broker.c Utils.c
StockMarket.o: StockMarket.c StockMarket.h
	gcc -c StockMarket.c Utils.c
clean:
	rm -r *.o
