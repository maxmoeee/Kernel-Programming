ifneq (${KERNELRELEASE},)
# call from kernel build system

obj-m := devnum.o hellodev.o mynull.o mylast.o scull.o

else

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

default:
	make -C ${KERNELDIR} M=${PWD} modules

clean:
	make -C ${KERNELDIR} M=${PWD} clean

endif
