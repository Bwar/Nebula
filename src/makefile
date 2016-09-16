CXX = g++
ROOT = .
STARTSHIP_MAKEFILES=$(shell find $(ROOT) -name 'makefile.*' -type f)

# make -f makefile.other -e unit_test=y

.PHONY : mks

mks: 
	@for make_file in $(STARTSHIP_MAKEFILES); do echo "making $$make_file"; make clean -f $$make_file; make -f $$make_file $(UNIT_TEST); done

clean:  
	@for make_file in $(STARTSHIP_MAKEFILES); do echo "cleaning in $$make_file"; make clean -f $$make_file; done

