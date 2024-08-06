all:
	cc src/steerbywire.c -o bin/steerbywire
	cc src/throttlecontrol.c -o bin/throttlecontrol
	cc src/brakebywire.c -o bin/brakebywire
	cc src/frontwindshieldcamera.c -o bin/frontwindshieldcamera
	cc src/forwardfacingradar.c -o bin/forwardfacingradar
	cc src/parkassist.c -o bin/parkassist
	cc src/output.c -o bin/output
	cc src/ECU.c -o bin/ECU

install:
	mkdir -p src
	mkdir -p bin
	mkdir -p logs
	mkdir -p data
	mv *.c src
	mv frontCamera.data data
	mv *.binary data
	$(MAKE) all


clean:
	mv src/* .
	mv data/* .
	rmdir src
	rmdir data
	rm -r bin
	rm -r logs