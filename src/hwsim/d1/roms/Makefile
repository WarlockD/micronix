#
# assemble the monitor roms and generate list and symbols for simulator
#
# hwsim/d1/roms/Makefile
#
# Changed: <2023-07-10 14:47:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

OBJ = mon375.bin mon447.bin
SYM = mon375.sym mon447.sym
SRC = mon375.s mon447.s

all: $(OBJ) $(SYM)

install: $(OBJ) $(SYM)
	cp $(OBJ) $(SYM) ..

%.bin: %.cim
	dd if=$< of=$@ bs=1024 skip=2 count=2
	#dd if=$< of=$@ bs=1024 skip=2 count=2 seek=2

%.lst %.cim: %.s
	../../../tools/zmac --oo lst,cim --od . -n -c $<

%.sym: %.lst
	cat $< | awk ' \
		/^Symbol Table/ { k=1 ; next } \
		/=/ { next } \
		(k == 1) { k++; next } \
		(k == 2) { printf "0x%04x %s\n", strtonum("0x" $$2), $$1; next } \
		' > $@

clean:
	rm -f *.cim *.lst *.bin *.sym

clobber: clean
