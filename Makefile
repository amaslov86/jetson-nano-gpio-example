all:    led switch ultrasonic 

led:	led.cpp
	g++ -g -O1 -o led led.cpp -Wall -std=gnu++17

switch:	switch.cpp
	g++ -g -O1 -o switch switch.cpp -Wall -std=gnu++17

ultrasonic:    ultrasonic.cpp
	g++ -g -O1 -o ultrasonic ultrasonic.cpp -Wall -std=gnu++17

clean:
	rm -f led led.o switch switch.o ultrasonic ultrasonic.o core
