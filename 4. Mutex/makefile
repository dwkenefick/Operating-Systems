all: freq potato primes

potato: potato.c ts.h cirq.c hash.c
	gcc -Wall -g -o potato.out potato.c cirq.c

primes: primes.c ts.h cirq.c hash.c
	gcc -Wall -g -o primes.out  primes.c cirq.c

freq: freq.c ts.h cirq.c hash.c
	gcc -Wall -g -o freq.out freq.c cirq.c

clean:
	rm *.out *dSYM