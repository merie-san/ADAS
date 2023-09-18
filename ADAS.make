bin_files := brake_by_wire ECUinput ECUoutput front_windshield_camera HMIinput HMIoutput park_assist steer_by_wire throttle_control surround_view_cameras

data_files := frontCamera.data urandomARTIFICIALE.binary

all: $(bin_files)

brake_by_wire: brake_by_wire.o readString.o
	cc -o $@ $^

ECUinput: ECUinput.o readString.o
	cc -o $@ $^

ECUoutput: ECUoutput.o readString.o
	cc -o $@ $^

front_windshield_camera: front_windshield_camera.o
	cc -o $@ $^

HMIinput: HMIinput.o
	cc -o $@ $^

HMIoutput: HMIoutput.o readString.o
	cc -o $@ $^

park_assist: park_assist.o
	cc -o $@ $^

steer_by_wire: steer_by_wire.o
	cc -o $@ $^

throttle_control: throttle_control.o readString.o
	cc -o $@ $^

surround_view_cameras: surround_view_cameras.o 
	cc -o $@ $^

%.o: %.c
	cc -c $<

install:
	rm *.o && mkdir bin && mv $(bin_files) ./bin &&\
	mkdir src && mv *.c *.h ./src && mkdir log && mv debuglog.txt ./log && mkdir data && mv $(data_files) ./data

clean:
	rm -f bin/*
	rm -f log/*

reinstall:
	mv ADAS.make ./src && cd src && $(MAKE) -f ADAS.make all && rm *.o &&\
	mv $(bin_files) ../bin && mv ADAS.make .. && cd .. && echo > log/debuglog.txt

.PHONY: all clean install clean reinstall
