#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define SCULL_MEM_SIZE 0x4000   /* 8KB for scull driver */
#define SCULL_IOCTL_MEM_CLEAR 1 /* Use for ioctl to clear memory */
#define SCULL_IOCTL_SHOW 10

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Yilin Fang, HUST IS1602");
MODULE_DESCRIPTION("A simple character driver module");

struct scull_dev
{
  struct cdev cdev; /* Char device structure */
  unsigned char mem[SCULL_MEM_SIZE];
  struct semaphore sem; /* mutual exclusion semaphore */
};

static struct scull_dev *scull_devp; /* Global dev pointer */
struct device *device;
struct class *class;
int scull_major = 0;
int scull_minor = 0;
/*
 * scull_open - Open the driver
 */
static int scull_open(struct inode *inodep, struct file *filep)
{
  printk(KERN_INFO "scull: Open\n");
  struct scull_dev *dev; /* device information */
  dev = container_of(inodep->i_cdev, struct scull_dev, cdev);
  filep->private_data = dev; /* for other methods */
  return 0;                  /* success */
}

/*
 * scull_release - Release the driver
 */
static int scull_release(struct inode *inodep, struct file *filep)
{
  printk(KERN_INFO "scull: Release\n");
  /*scull is a virtual device, don't need to shutdown the hardware*/
  return 0;
}

/*
 * scull_read - Read from the driver
 */
static ssize_t scull_read(struct file *filep, char __user *buf, size_t count, loff_t *offset)
{
  printk(KERN_INFO "scull: Start read\n");
  struct scull_dev *dev = filep->private_data;
  if (down_interruptible(&dev->sem))
    return -ERESTARTSYS;
  int ret = 0;
  size_t avail = SCULL_MEM_SIZE - *offset;
  /* Available memory exists */
  if (count <= avail)
  {
    if (copy_to_user(buf, dev->mem + *offset, count) != 0)
      return -EFAULT;
    *offset += count;
    ret = count;
  }
  /* Available memory not enough */
  else
  {
    if (copy_to_user(buf, dev->mem + *offset, avail) != 0)
      return -EFAULT;
    *offset += avail;
    ret = avail;
  }
  up(&dev->sem);
  printk(KERN_INFO "scull: Read %u bytes\n", ret);
  return ret;
}

/*
 * scull_write - Write from the driver
 */
static ssize_t scull_write(struct file *filep, const char __user *buf, size_t count, loff_t *offset)
{
  printk(KERN_INFO "scull: Start write\n");
  struct scull_dev *dev = filep->private_data;
  if (down_interruptible(&dev->sem))
    return -ERESTARTSYS;
  printk(KERN_INFO "scull: After write\n");
  int ret = 0;
  size_t avail = SCULL_MEM_SIZE - *offset;
  /* Available memory exists */
  if (count > avail)
  {
    if (copy_from_user(dev->mem + *offset, buf, avail) != 0)
      return -EFAULT;
    *offset += avail;
    ret = avail;
  }
  /* Available memory not enough */
  else
  {
    if (copy_from_user(dev->mem + *offset, buf, count) != 0)
      return -EFAULT;
    *offset += count;
    ret = count;
  }
  printk(KERN_INFO "scull: written %u bytes\n", ret);
  up(&dev->sem);
  return ret;
}

/*
 * scull_llseek - Set the current position of the file for reading and writing
 */
static loff_t scull_llseek(struct file *filep, loff_t offset, int whence)
{
  printk(KERN_INFO "scull: Start llseek\n");
  loff_t ret = 0;
  struct scull_dev *dev = filep->private_data;
  switch (whence)
  {
  /* SEEK_SET */
  case 0:
    if (offset < 0)
    {
      ret = -EINVAL;
      break;
    }
    if (offset > SCULL_MEM_SIZE)
    {
      ret = -EINVAL;
      break;
    }
    ret = offset;
    break;
  /* SEEK_CUR*/
  case 1:
    if ((filep->f_pos + offset) > SCULL_MEM_SIZE)
    {
      ret = -EINVAL;
      break;
    }
    if ((filep->f_pos + offset) < 0)
    {
      ret = -EINVAL;
      break;
    }
    ret = filep->f_pos + offset;
    break;
  default:
    ret = -EINVAL;
  }

  if (ret < 0)
    return ret;

  printk(KERN_INFO "scull: Set offset to %u\n", ret);
  filep->f_pos = ret;
  return ret;
}

/*
 * scull_ioctl - Control the scull driver(memory clear)
 */
static long scull_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
  printk(KERN_INFO "scull: Start ioctl process\n");
  int ret, tmp;
  int i, len;
  struct scull_dev *dev = filep->private_data;
  switch (cmd)
  {
  case SCULL_IOCTL_MEM_CLEAR:
    memset(dev->mem, 0, SCULL_MEM_SIZE);
    printk(KERN_INFO "scull: Memory is set to zero\n");
    break;
  case SCULL_IOCTL_SHOW:
    printk(KERN_INFO "scull: Show memory\n");
    len = strlen(dev->mem);
    for (i = 0; i < len; i++)
    {
      printk("scull: %d:%c", i, dev->mem[i]);
    }
    break;
  default:
    return -EINVAL;
  }
  return 0;
}

/*
 * Set operation pointers
 */
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = scull_open,
    .release = scull_release,
    .read = scull_read,
    .write = scull_write,
    .llseek = scull_llseek,
    .unlocked_ioctl = scull_ioctl,
};

/*
 * scull_init - Initial function for scull
 */
static int __init scull_init(void)
{
  printk(KERN_INFO "scull: Load module\n");

  int ret;
  dev_t devno;
  if (scull_major)
  {
    devno = MKDEV(scull_major, scull_minor);
    ret = register_chrdev_region(devno, 1, "scull");
  }
  else
  {
    ret = alloc_chrdev_region(&devno, scull_minor, 1, "scull");
    scull_major = MAJOR(devno);
  }
  if (ret < 0)
  {
    printk(KERN_ALERT "scull: Can't get major %d\n", scull_major);
    return ret;
  }

  /* Alloc memory for device */
  scull_devp = kzalloc(sizeof(struct scull_dev), GFP_KERNEL);
  if (scull_devp == NULL)
  {
    printk(KERN_ALERT "scull: Alloc memory for device failed\n");
    ret = -ENOMEM;
    goto failed;
  }
  /* Setup device */
  sema_init(&scull_devp->sem, 1);
  cdev_init(&scull_devp->cdev, &fops);
  scull_devp->cdev.owner = THIS_MODULE;
  cdev_add(&scull_devp->cdev, devno, 1);

  /* Creat device file */
  class = class_create(THIS_MODULE, "scull");
  if (IS_ERR(class))
  {
    ret = PTR_ERR(class);
    printk(KERN_ALERT "scull: Creat class for device file failed with %d\n", ret);
    goto failed;
  }
  device = device_create(class, NULL, devno, NULL, "scull");
  if (IS_ERR(device))
  {
    class_destroy(class);
    ret = PTR_ERR(device);
    printk(KERN_ALERT "scull: Creat device file failed with %d\n", ret);
    goto failed;
  }

  return 0;

failed:
  unregister_chrdev_region(devno, 1);
  return ret;
}

/*
 * scull_exit - Exit function for scull
 */
static void __exit scull_exit(void)
{
  printk(KERN_INFO "scull: Unload module\n");
  dev_t devno = MKDEV(scull_major, scull_minor);
  device_destroy(class, devno);
  class_unregister(class);
  class_destroy(class);
  cdev_del(&scull_devp->cdev);
  kfree(scull_devp);
  unregister_chrdev_region(devno, 1);
}

module_init(scull_init);
module_exit(scull_exit);
