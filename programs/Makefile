AS86 = as86 -l
LD86 = ld86 -0 -s

all: emufs.sys

%.sys: %.S
	$(AS86) -0 -o $*.o $< > $<.out
	$(LD86) -T 0 -s -o $*.tmp $*.o
	dd if=$*.tmp of=$@ bs=1 skip=32
	rm $*.tmp $*.o

clean:
	rm -f emufs.sys *.o *.tmp *.out
