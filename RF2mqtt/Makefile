
# Defines the RPI variable which is needed by rc-switch/RCSwitch.h
CXXFLAGS=-DRPI

all: RF2mqtt

RF2mqtt: ../rc-switch/RCSwitch.o RF2mqtt.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi -lwiringPiDev -lcrypt -lmosquittopp -lpugixml

clean:
	$(RM) ../rc-switch/*.o *.o RF2mqtt



