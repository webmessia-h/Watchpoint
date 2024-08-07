
obj-m += src/watchpoint.o

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(src) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(src) clean

