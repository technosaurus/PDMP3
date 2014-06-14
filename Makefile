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
CFLAGS = -O4 -funroll-loops -Wall -ansi -DOUTPUT_RAW 
#CFLAGS = -O4 -funroll-loops -Wall -ansi -DOUTPUT_DBG

OBJS = MP3_Bitstream.o MP3_Decoder.o MP3_Huffman.o MP3_Huffman_Table.o \
	MP3_Main.o MP3_Synth_Table.o main.o remote_control.o audio.o debug.o

OPT_OBJS = MP3_Bitstream.o MP3_Decoder_opt.o MP3_Huffman.o \
	MP3_Huffman_Table.o MP3_Main.o MP3_Synth_Table.o \
	main.o remote_control.o audio.o debug.o

all: mp3decode mp3dec_opt bcmp

#
# bcmp compares two PCM files, and counts how many samples differ 
# by 1 (small errors), and 2 or more (big errors). This is used to
# verify the accuracy of the decoder compared to the IIS reference
# implementation.
#
bcmp: bcmp.c
	$(CC) $(CFLAGS) -o bcmp bcmp.c -lm

mp3decode: $(OBJS)
	$(CC) $(CFLAGS) -o mp3decode  $(OBJS) -lm
	@echo
	@echo "********** Made mp3decode **********"
	@echo

mp3dec_opt: $(OPT_OBJS)
	$(CC) $(CFLAGS) -o mp3dec_opt  $(OPT_OBJS) -lm
	@echo
	@echo "********** Made mp3dec_opt **********"
	@echo

#
# Install the decoder and utilities to /usr/local/bin.
# This probably needs to be done as root.
#
install: mp3decode mp3dec_opt bcmp
	cp mp3decode mp3dec_opt bcmp /usr/local/bin

depend: 
	gcc -MM $(CFLAGS) *.c > make.depend

clean: 
	-rm -f *.o *~ core TAGS *.wav *.bin

realclean: clean
	-rm -f mp3decode mp3dec_opt *.pdf *.ps bcmp mcmp *.bit

etags:
	etags *.c *.h

print:
	-rm -f app_b.ps
	a2ps --medium=a4 -G2r --left-title="" \
	--header="Appendix B, Reference Decoder and Simulation Source Code" \
	--file-align=fill  \
	main.c -oapp_b.ps MP3_Main.h MP3_Main.c MP3_Huffman_opt.c        \
	MP3_Decoder.h MP3_Decoder_opt.c requant_sim.c
	-rm -f app_b.pdf
	ps2pdf app_b.ps

include make.depend
