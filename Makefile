all:    mem ultrasonic 

mem:	gpiomem.cpp
	g++ -g -O1 -o mem gpiomem.cpp -Wall -std=gnu++17

ultrasonic:    ultrasonic.cpp
	g++ -g -O1 -o ultrasonic ultrasonic.cpp -Wall -std=gnu++17

clean:
	rm -f mem gpiomem.o ultrasonic ultrasonic.o core
