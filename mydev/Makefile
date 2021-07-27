MAKEFILE_DIR := $(shell cd $(dir $(lastword $(MAKEFILE_LIST))); pwd)

help:
	@echo "the Jetson gpios device driver installer, liu ya fei"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

build: ## build the Jetson gpios kernel module, mygpios.ko
	cd $(MAKEFILE_DIR)/drivers/mygpios && make mygpios.ko

clean: ## clean the object files created while building the kernel module
	cd $(MAKEFILE_DIR)/drivers/mygpios && make clean

install: build ## install mygpios.ko and set auto load
	cp 50-mygpios.rules /etc/udev/rules.d/
	cp $(MAKEFILE_DIR)/drivers/mygpios/mygpios.ko /lib/modules/`uname -r`/kernel/drivers/misc/
	depmod -A
	modprobe mygpios
	echo mygpios | sudo tee /etc/modules-load.d/mygpios.conf > /dev/null

uninstall: ## remove mygpios.ko and un-set auto load
	-modprobe -r mygpios
	rm /etc/udev/rules.d/50-mygpios.rules
	rm /etc/modules-load.d/mygpios.conf
	rm /lib/modules/`uname -r`/kernel/drivers/misc/mygpios.ko

insmod: build ## insmod mygpios.ko
	sudo insmod $(MAKEFILE_DIR)/drivers/mygpios/mygpios.ko
	sleep 1
	-sudo chmod 666 /dev/mygpio*
	

rmmod: ## rmmod mygpios.ko
	sudo rmmod mygpios

_dmesg:
	dmesg -x --color -l emerg,alert,crit,err,info,debug
