#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include<linux/slab.h>                 
#include<linux/uaccess.h>             
#include<linux/kobject.h> 
#include <linux/interrupt.h>
#include <asm/io.h>
#include<linux/kthread.h>
#include<linux/delay.h>
#include<linux/jiffies.h>
#include<linux/sysfs.h> 
#include<linux/timer.h> 
#include<linux/ioctl.h> 
#include <linux/workqueue.h> 
#include <linux/jiffies.h>

#define MAXLEN 1024
#define IRQ_NO 6
char *kernel_buffer; 
//ankit sahu

//main test
//branch test
//test again


dev_t dev = 0;
int count=0;
static void work_queue_fun(struct work_struct *work);
static struct class *dev_class;
static struct cdev chr_cdev;
static struct delayed_work dwork;

static int __init chr_driver_init(void);
static void __exit chr_driver_exit(void);
 
/*************** Driver Fuctions **********************/
static int chr_open(struct inode *inode, struct file *file);
static int chr_release(struct inode *inode, struct file *file);
static ssize_t chr_read(struct file *filp,char __user *buf, size_t len,loff_t * off);
static ssize_t chr_write(struct file *filp,const char *buf, size_t len, loff_t * off); 
/******************************************************/

char att_value[1024];
struct kobject *kobj; 
char *buff;
int fw;
/*************** Sysfs Fuctions **********************/
static ssize_t sysfs_show(struct kobject *kobj, 
                struct kobj_attribute *attr, char *buf);
static ssize_t sysfs_store(struct kobject *kobj, 
                struct kobj_attribute *attr,const char *buf, size_t count);
 
struct kobj_attribute att_attr = __ATTR(att_value, 0660, sysfs_show, sysfs_store);
/*****************************************************/

static irqreturn_t irq_handler(int irq,void *dev_id)
{
	
	
	printk(KERN_INFO "Interrup invoked  times");
	//schedule_delayed_work_on(0, &dwork, 5*HZ);
	return IRQ_HANDLED;
}

void work_queue_fun(struct work_struct *work)
{
	printk("Workqueue function being executed");
	
        sprintf(buff, "%s", "Test string");
	sysfs_store(kobj, &att_attr, buff, 1024);
	
	sysfs_show(kobj, &att_attr, buff);
	printk(KERN_INFO "Data read from sysfs attribute is :  %s\n", buff);
	
}

static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = chr_read,
        .write          = chr_write,
        .open           = chr_open,
        .release        = chr_release,
};

static ssize_t sysfs_show(struct kobject *kobj, 
                struct kobj_attribute *attr, char *buf)
{
        printk(KERN_INFO "Sysfs - Read!!!\n");
        printk(KERN_INFO "Sysfs - Read data: %s\n", (char *)kobj->name);
        return sprintf(buf, "%s", kobj->name);
}

/*
** This function will be called when we write the sysfsfs file
*/
static ssize_t sysfs_store(struct kobject *kobj, 
                struct kobj_attribute *attr,const char *buf, size_t count)
{
        printk(KERN_INFO "Sysfs - Write!!!\n");
       
        printk(KERN_INFO "Sysfs -Write data: %s\n", buf);
        return sprintf((char *) kobj->name, "%s", buf);
}

static int chr_open(struct inode *inode, struct file *file)
{
        /*(KERN_INFO "Device File Opened...!!!\n");*/
        return 0;
}
 
static int chr_release(struct inode *inode, struct file *file)
{
        /*printk(KERN_INFO "Device File Closed...!!!\n");*/
        return 0;
}
 
static ssize_t chr_read(struct file *fp,char __user *buff, size_t length, loff_t *ppos)
{
	
	/*printk(KERN_INFO "Inside read function, current offset %lld and length is %d",*ppos, (int)length);*/

        int bytes_read = length - copy_to_user(buff, kernel_buffer, length);
      	
        return bytes_read;	
}

static ssize_t chr_write(struct file *fp,const char __user *buff, size_t length, loff_t *ppos)
{

        int bytes_written = length - copy_from_user(kernel_buffer + *ppos, buff, length);
        
        printk(KERN_INFO "invoked\n");
      	printk(KERN_INFO "Data written : %sFrom offset %lld\n", kernel_buffer+*ppos, *ppos);
      	*ppos += (long long int)length;
        return bytes_written;
}
 
static int __init chr_driver_init(void)
{
        /*Allocating Major number*/
        if((alloc_chrdev_region(&dev, 0, 1, "chr_Dev")) <0){
                printk(KERN_INFO "Cannot allocate major number\n");
                return -1;
        }
        /*printk(KERN_INFO "Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));*/
 
        /*Creating cdev structure*/
        cdev_init(&chr_cdev,&fops);
 
        /*Adding character device to the system*/
        if((cdev_add(&chr_cdev,dev,1)) < 0){
            printk(KERN_INFO "Cannot add the device to the system\n");
            goto r_class;
        }
 
        /*Creating struct class*/
        if((dev_class = class_create(THIS_MODULE,"chr_class")) == NULL){
            printk(KERN_INFO "Cannot create the struct class\n");
            goto r_class;
        }
 
        /*Creating device*/
        if((device_create(dev_class,NULL,dev,NULL,"chr_device")) == NULL){
            printk(KERN_INFO "Cannot create the Device 1\n");
            goto r_device;
        }

	/* allocating space for the kernel buffer */
        kernel_buffer=kmalloc(MAXLEN ,GFP_KERNEL);
        buff=kmalloc(MAXLEN ,GFP_KERNEL);
        kobj = kobject_create_and_add("etx_sysfs",kernel_kobj);
 
        /*Creating sysfs file for etx_value*/
        sysfs_create_file(kobj,&att_attr.attr);
        
        request_irq(IRQ_NO, irq_handler, NULL, "chr_device",NULL);
	INIT_DELAYED_WORK(&dwork, work_queue_fun);
	printk(KERN_INFO "%s", kobj->name);
        printk(KERN_INFO "Device Driver Insert...Done!!!\n");
    return 0;

irq:
	free_irq(IRQ_NO,(void *)(irq_handler)); 
r_sysfs:
        kobject_put(kobj); 
        sysfs_remove_file(kernel_kobj, &att_attr.attr); 
r_device:
        class_destroy(dev_class);
r_class:
        cdev_del(&chr_cdev);
        unregister_chrdev_region(dev,1);
        return -1;
}
 
void __exit chr_driver_exit(void)
{
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
	cdev_del(&chr_cdev);
        unregister_chrdev_region(dev, 1);
	kfree(kernel_buffer);
        printk(KERN_INFO "Device Driver Remove...Done!!!\n");
        /*free_irq(IRQ_NO,(void*)(irq_handler));*/
        kobject_put(kobj); 
        sysfs_remove_file(kernel_kobj, &att_attr.attr);
        flush_delayed_work(&dwork);
}
 
module_init(chr_driver_init);
module_exit(chr_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Freshers");
MODULE_DESCRIPTION("Example Character Device Driver ");