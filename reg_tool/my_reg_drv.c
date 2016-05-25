#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>

struct reg {
  unsigned int addr;
  unsigned int data;
};

static int major;
static struct class *cls;

static int my_reg_open(struct inode *inode, struct file *file)
{
	printk("my_reg_drv open\n");
	return 0;	
}

static ssize_t my_reg_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	struct reg val;
	unsigned int *virtual_add;
    if (copy_from_user(&val, buf, count)) {
        printk(KERN_ERR"my_reg_write:¡ccopy_from_user error\n");
        return -1;
    }
    virtual_add = ioremap(val.addr, 32);
	if (!virtual_add){
		printk(KERN_ERR"my_reg_write:¡ccould not remap register memory\n");
		return -1;
	}
	*virtual_add = val.data;
	iounmap(virtual_add);
	return 0;
}

ssize_t my_reg_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    struct reg val;
    unsigned int *virtual_add;
    if (copy_from_user(&val, buf, size)) {
        printk(KERN_ERR"my_reg_read:¡ccopy_from_user error\n");
        return -1;
    }
    virtual_add = ioremap(val.addr, 32);
    if (!virtual_add){
        printk(KERN_ERR"my_reg_read:¡ccould not remap register memory\n");
        return -1;
    }
    val.data = *virtual_add;
    if (copy_to_user(buf, &val, size)) {
            printk(KERN_ERR"my_reg_read:¡copy_to_user error\n");
            return -1;
        }
	iounmap(virtual_add);
	return 0;
}


static struct file_operations my_reg_fops = {
    .owner  =   THIS_MODULE,    
    .open   =   my_reg_open,     
	.write	=	my_reg_write,
	.read   =   my_reg_read,
};


static int my_reg_drv_init(void)
{
	printk("enter my_reg_drv_init\n");

	major = register_chrdev(0, "my_reg", &my_reg_fops);

	cls = class_create(THIS_MODULE, "my_reg");

	device_create(cls, NULL, MKDEV(major, 0), NULL, "my_reg"); /* /dev/my_reg */
	
	return 0;
}

static void my_reg_drv_exit(void)
{
		printk("enter my_reg_remove\n");

	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	unregister_chrdev(major, "my_reg");
}

module_init(my_reg_drv_init);
module_exit(my_reg_drv_exit);

MODULE_LICENSE("GPL");



