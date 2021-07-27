#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>


MODULE_AUTHOR("liuyafei");
MODULE_DESCRIPTION("A device driver of Jetson agx xavier to gpio");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.3");


/* --- GPIO Pins --- */
//先定义两个gpio口
//这里对应的是Jetson-Xavier-GPIO-mapping.xlsx的linux gpio那一列
#define GPIO_OP0 251	 // PIN29
#define GPIO_OP1 250	 // PIN31


/*定义主设备号和次设备号*/
#define DEV_MAJOR 0
#define DEV_MINOR 0

#define REG_GPIO_NAME "Jetson GPIO"
/*定义gpio数量，为了方便加入iic和spi设备驱动，所以加了一个宏定义*/
#define NUM_DEV_GPIO 2
#define NUM_DEV_TOTAL NUM_DEV_GPIO
/*设备名*/
#define DEVNAME_GPIO "mygpio"
#define DRIVER_NAME "mygpios"
/*创建gpios类，方便自动创建设备节点*/
static struct class *class_opgpio = NULL;
static struct mutex lock;
static int _major_opgpio = DEV_MAJOR;
static int _minor_opgpio = DEV_MINOR;
/*设备号数组*/
static struct cdev *cdev_array = NULL;
/*设备号索引*/
static volatile int cdev_index = 0;

/*gpio输出1*/
static int dev_put(int ledno)
{
	switch (ledno) {
	case 0:
		gpio_set_value(GPIO_OP0, 1);
		printk("gpio29输出1!\n");
		break;
	case 1:
		gpio_set_value(GPIO_OP1, 1);
		printk("gpio31输出1!\n");
		break;
	}
	return 0;
}
/*gpio输出0*/
static int dev_del(int ledno)
{
	switch (ledno) {
	case 0:
		gpio_set_value(GPIO_OP0, 0);
		printk("gpio29输出0!\n");
		break;
	case 1:
		gpio_set_value(GPIO_OP1, 0);
		printk("gpio29输出0!\n");
		break;
	}

	return 0;
}
/*write操作函数*/
static ssize_t dev_write(struct file *filep, const char __user *buf,
			 size_t count, loff_t *f_pos)
{
	//cval为内核空间里的字符
	char cval;
	int ret;
	int minor = *((int *)filep->private_data);
	//将用户空间输出的字符buf复制到内核空间里的cval
	if (count > 0) {
		if (copy_from_user(&cval, buf, sizeof(char))) {
			return -EFAULT;
		}
		switch (cval) {
		case '1':
			ret = dev_put(minor);
			break;
		case '0':
			ret = dev_del(minor);
			break;
		}
		return sizeof(char);
	}
	return 0;
}

static int dev_open(struct inode *inode, struct file *filep)
{
	int *minor = (int *)kmalloc(sizeof(int), GFP_KERNEL);
	//得到次设备号
	// int major = MAJOR(inode->i_rdev);
	*minor = MINOR(inode->i_rdev);
	// printk(KERN_INFO "open request major:%d minor: %d \n", major,
	// *minor);
	//将次设备号保存设置为私有数据
	filep->private_data = (void *)minor;
	return 0;
}

static int dev_release(struct inode *inode, struct file *filep)
{
	kfree(filep->private_data);
	return 0;
}
/*设备操作函数*/
static struct file_operations mygpios_fops = {
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
};

	
/*gpio注册函数*/
static int mygpio_register_dev(void)
{
	int retval;
	dev_t dev;
	dev_t devno;
	int i;
	/*没有定义设备号，申请设备号*/
	retval = alloc_chrdev_region(&dev, DEV_MINOR, NUM_DEV_GPIO, DEVNAME_GPIO);

	if (retval < 0) {
		printk(KERN_ERR "%s: alloc_chrdev_region failed.\n", __func__);
		return retval;
	}
	/*主设备号*/
	_major_opgpio = MAJOR(dev);
	/*创建设备*/
	class_opgpio = class_create(THIS_MODULE, DEVNAME_GPIO);
	if (IS_ERR(class_opgpio)) {
		return PTR_ERR(class_opgpio);
	}
	/*存在多个gpio次设备,循环进行初始化cdev，添加dev*/
	for (i = 0; i < NUM_DEV_GPIO; i++) {
		devno = MKDEV(_major_opgpio, _minor_opgpio + i);
		//初始化cdev
		cdev_init(&(cdev_array[cdev_index]), &mygpios_fops);
		cdev_array[cdev_index].owner = THIS_MODULE;
		//添加一个cdev
		if (cdev_add(&(cdev_array[cdev_index]), devno, 1) < 0) {
			printk(KERN_ERR "%s: cdev_add failed minor = %d\n",
			       __func__, _minor_opgpio + i);
		} else {
			device_create(class_opgpio, NULL, devno, NULL,
				      DEVNAME_GPIO "%u", _minor_opgpio + i);
		}
		cdev_index++;
	}
	return 0;
}

static int __init mygpio_init(void)
{
	int retval = 0;
	int registered_devices = 0;
	size_t size;

	printk(KERN_INFO "%s: loading %d devices...\n", DRIVER_NAME,
	       NUM_DEV_TOTAL);

	mutex_init(&lock);

	if (!gpio_is_valid(GPIO_OP0)) {
		printk(KERN_INFO "GPIO: invalid LED0 GPIO\n");
		return -ENODEV;
	}
	if (!gpio_is_valid(GPIO_OP1)) {
		printk(KERN_INFO "GPIO: invalid LED1 GPIO\n");
		return -ENODEV;
	}
	//请求gpio
	retval = gpio_request(GPIO_OP0, "sysfs");
	retval = gpio_request(GPIO_OP1, "sysfs");
    //设置gpio为输出
	retval = gpio_direction_output(GPIO_OP0, 0);
	retval = gpio_export(GPIO_OP0, 0);

	retval = gpio_direction_output(GPIO_OP1, 0);
	retval = gpio_export(GPIO_OP1, 0);

	size = sizeof(struct cdev) * NUM_DEV_TOTAL;
	cdev_array = (struct cdev *)kmalloc(size, GFP_KERNEL);

	retval = mygpio_register_dev();
	if (retval != 0) {
		printk(KERN_ALERT "%s: led driver register failed.\n",
		       DRIVER_NAME);
		return retval;
	}
	printk(KERN_INFO "%s: %d devices loaded.\n", DRIVER_NAME,
	       registered_devices + NUM_DEV_TOTAL);
	
	return 0;
}

static void __exit mygpio_exit(void)
{
	int i;
	dev_t devno;
	dev_t devno_top;

	printk(KERN_DEBUG "%s: removing %d cdev(s).\n", DRIVER_NAME,
	       NUM_DEV_TOTAL);
	for (i = 0; i < NUM_DEV_TOTAL; i++) {
		cdev_del(&(cdev_array[i]));
	}

	/* /dev/rtled0,1,2,3 */
	devno_top = MKDEV(_major_opgpio, _minor_opgpio);
	for (i = 0; i < NUM_DEV_GPIO; i++) {
		devno = MKDEV(_major_opgpio, _minor_opgpio + i);
		device_destroy(class_opgpio, devno);
	}
	unregister_chrdev_region(devno_top, NUM_DEV_GPIO);

	
	class_destroy(class_opgpio);


	kfree(cdev_array);

	mutex_destroy(&lock);

	/* GPIO unmap */
	/* set all gpio as low */
	gpio_set_value(GPIO_OP0, 0);
	gpio_set_value(GPIO_OP1, 0);
	/* sysfs: reverses the effect of exporting to userspace */
	gpio_unexport(GPIO_OP0);
	gpio_unexport(GPIO_OP1);
	/* reverse gpio_export() */
	gpio_free(GPIO_OP0);
	gpio_free(GPIO_OP1);
	printk("module being removed at %lu\n", jiffies);
}

module_init(mygpio_init);
module_exit(mygpio_exit);


