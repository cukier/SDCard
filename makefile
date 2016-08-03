CC = ccsc
PK2 = pk2cmd

DEVICE = PIC18F46K22

SRC = src
OUT = Debug
REL = Release

UNIT = SDCARD
UINT_FILE = main

OBJS += *.ccspjt *.cof *.err *.esym *.hex *.lst *.xsym
MOBJ = $(OBJS:%=$(SRC)/%)

ifeq ($(DEVICE), PIC18F46K22)
CFLAGS += +FH
endif
ifeq ($(DEVICE), PIC18F25K22)
CFLAGS += +FH
endif

CFLAGS += +LN -T -A -M -Z +DF +Y=9 +STDOUT +EA
PK2DELFLAGS += -E -P$(DEVICE)
PK2FLAGS +=$(PK2DELFLAGS) -M -R -J -F

all: clean $(UNIT)

$(UNIT): $(SRC)/$(UINT_FILE).c
	$(CC) $(CFLAGS) $(DFLAGS) $<
	[[ -d $(OUT) ]] || mkdir $(OUT)
	mv $(MOBJ) $(OUT)
	
burn: $(OUT)/$(UINT_FILE).hex
	$(PK2) $(PK2FLAGS) $<
	
erase:
	$(PK2) $(PK2DELFLAGS)
	
clean:
	rm -Rvf $(OUT)
	
clean_release:
	rm -Rvf $(REL)
