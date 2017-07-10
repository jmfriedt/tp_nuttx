#include <nuttx/config.h>
#include <nuttx/arch.h>
#include <nuttx/spi/spi.h>

#include <stdio.h>

typedef FAR struct file		file_t;

static int     dummy_open(file_t *filep);
static int     dummy_close(file_t *filep);
static ssize_t dummy_read(file_t *filep, FAR char *buffer, size_t buflen);
static ssize_t dummy_write(file_t *filep, FAR const char *buf, size_t buflen);

static const struct file_operations dummy_ops = {
	dummy_open,		/* open */
	dummy_close,		/* close */
	dummy_read,		/* read */
	dummy_write,		/* write */
	0,			/* seek */
	0,			/* ioctl */
};

static int dummy_open(file_t *filep)
{
  printf("Open\n");
  return OK;
}

static int dummy_close(file_t *filep)
{
  printf("Close\n");
  return OK;
}

static ssize_t dummy_read(file_t *filep, FAR char *buf, size_t buflen)
{int k;
 if (buflen>10) buflen=10;
 for (k=0;k<buflen;k++) {buf[k]='0'+k;}
 printf("Read %d\n",buflen);
 return buflen;
}

static ssize_t dummy_write(file_t *filep, FAR const char *buf, size_t buflen)
{int k;
 printf("Write %d - ",buflen);
 for (k=0;k<buflen;k++) printf("%c",buf[k]);
 printf("\n");
 return buflen;
}

/****************************************************************************
 * Initialize device, add /dev/... nodes
 ****************************************************************************/
void stm32_dummy_setup(void)
{
  (void)register_driver("/dev/dummy", &dummy_ops, 0444, NULL);
}

/*
no SPI driver -- SPI is a bus
http://nuttx.org/doku.php?id=wiki:nxinternal:devices-vs-buses

je DOIS activer dans f401-nsh/defconfig : CONFIG_BOARD_INITIALIZE=y (qui etait unset)
puis dans src/stm32_boot.c : void board_initialize(void)
j'ajoute stm32_adc_setup(); stm32_dummy_setup();

nsh> ls -l /dev
/dev:
 cr--r--r--       0 adc0
 crw-rw-rw-       0 console
 crw-rw-rw-       0 dummy
 crw-rw-rw-       0 null
 crw-rw-rw-       0 ttyS0
nsh> cat < /dev/ttyS0
nsh: cat: open failed: 2
nsh> echo "1" > /dev/dummy
Open
Write
Write
Write
Close
nsh> cat < /dev/dummy
nsh: cat: open failed: 2
nsh> dd if=/dev/dummy of=/dev/console bs=5 count=1
Open
Read 5
01234Close

*/
