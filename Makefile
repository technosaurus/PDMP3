#
# Linux
#
CC = gcc

#
# Flags:
#
# DEBUG_CHECK     Extra checking of bitstream data
# OUTPUT_SOUND    Write sound data to /dev/dsp
# OUTPUT_RAW      Write sound data to <filename>.raw
# OUTPUT_DBG      Write clear-text debug dumps to stdout

#CFLAGS = -g -O4 -funroll-loops -Wall -ansi -DOUTPUT_SOUND
#CFLAGS = -O4 -funroll-loops -Wall -ansi -DOUTPUT_RAW 
#CFLAGS = -O4 -funroll-loops -Wall -ansi -DOUTPUT_DBG
CFLAGS = -Os -ffunction-sections -fdata-sections \
	-finline-small-functions -finline-functions-called-once \
	-fno-unwind-tables -fno-asynchronous-unwind-tables \
	-ffast-math -fassociative-math -fomit-frame-pointer -ffinite-math-only \
	-fno-math-errno -fno-trapping-math -freciprocal-math -frounding-math \
	-funsafe-loop-optimizations -funsafe-math-optimizations \
	-DOUTPUT_SOUND -DIMDCT_TABLES -DIMDCT_NTABLES -DPOW34_TABLE
LDFLAGS = -Wl,--gc-sections,--as-needed,-s

OBJS = pdmp3.o main.o

all: pdmp3

pdmp3: $(OBJS)
	$(CC) $(CFLAGS) -o pdmp3  $(OBJS) $(LDFLAGS) -lm
	@echo
	@echo "********** Made pdmp3 **********"
	@echo


#
# Install the decoder and utilities to /usr/local/bin.
# This probably needs to be done as root.
#
install: pdmp3
	cp pdmp3 /usr/local/bin

depend: 
	gcc -MM $(CFLAGS) *.c > make.depend

clean: 
	-rm -f *.o *~ core TAGS *.wav *.bin

realclean: clean
	-rm -f pdmp3 *.pdf *.ps *.bit

etags:
	etags *.c *.h

print:
	-rm -f app_b.ps
	a2ps --medium=a4 -G2r --left-title="" \
	--header="Appendix B, Reference Decoder and Simulation Source Code" \
	--file-align=fill  \
	main.c -oapp_b.ps pdmp3.c
	-rm -f app_b.pdf
	ps2pdf app_b.ps


