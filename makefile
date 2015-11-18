CC=ccsc
PK2=pk2cmd

DEVICE=18f25K22
UNIT=SDCard
UNIT_FILE=main

CFLAGS=+FH +ES +J +DC +LN -Z +DF +Y=9 +STDOUT +EA
PK2FLAGS=-E -PPIC$(DEVICE) -M -R -J -F

all: $(UNIT)

$(UNIT): src/$(UNIT_FILE).c
	$(CC) src/$(UNIT_FILE).c $(CFLAGS)
	mv src/*.ccspjt src/*.cof src/*.err src/*.esym src/*.hex src/*.lst src/*.sym src/*.xsym Debug

burn :
	$(PK2) $(PK2FLAGS) Debug/$(UNIT_FILE).hex

clean:
	rm Debug/*