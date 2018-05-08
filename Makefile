obj-m+= my_module.o

NM_SRC?= $(PWD)/../netmap
K_SRC?= /lib/modules/$(shell uname -r)/build
SRCDIR= $(PWD)

EXTRA_CFLAGS := -I$(NM_SRC) -I$(NM_SRC)/LINUX -I$(NM_SRC)/sys -DCONFIG_NETMAP
EXTRA_CFLAGS += -Wno-unused-but-set-variable -g

all:	build

build:
	make -C $(K_SRC) M=$(PWD) \
		CONFIG_NETMAP=m CONFIG_NETMAP_VALE=y CONFIG_MYMODULE=m \
		EXTRA_CFLAGS='$(EXTRA_CFLAGS)' KBUILD_EXTRA_SYMBOLS=$(NM_SRC)/Module.symvers

clean:
	(rm -rf *.o *.ko modules.order mymodule_lin.mod.c Module.symvers)