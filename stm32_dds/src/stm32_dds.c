#include <nuttx/config.h>
#include <nuttx/arch.h>
#include <nuttx/spi/spi.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/spi/spi.h>
#include <arch/board/board.h>

void envoi_DDS (unsigned short );

typedef FAR struct file		file_t;

static int     dds_open(file_t *);
static int     dds_close(file_t *);
static ssize_t dds_read(file_t *, FAR char *, size_t );
static ssize_t dds_write(file_t *, FAR const char *, size_t );
static int     dds_ioctl(FAR struct file *, int , unsigned long );

static const struct file_operations dds_ops = {
	dds_open,		/* open */
	dds_close,		/* close */
	dds_read,		/* read */
	dds_write,		/* write */
	0,			/* seek */
	dds_ioctl,		/* ioctl */
};

FAR struct spi_dev_s *spi;

#define GPIO_RESET_DDS \
    (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHz | \
     GPIO_OUTPUT_SET | GPIO_PORTA | GPIO_PIN15)

#define GPIO_SLEEP_DDS \
    (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHz | \
     GPIO_OUTPUT_SET | GPIO_PORTA | GPIO_PIN14)

#define GPIO_DET \
    (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHz | \
     GPIO_OUTPUT_SET | GPIO_PORTC | GPIO_PIN10)

static int dds_open(file_t *filep)
{ unsigned short ctrl_nr=0x2028;            // 16 bit ctrl without reset
  unsigned short ctrl_r =0x2128;            // 16 bit ctrl with reset
  unsigned short phase0 =0xc000; // phase = 0
  unsigned short phase1 =0xe000; // phase = 0
  printf("Open\n");

  stm32_spidev_initialize();
  spi=stm32_spibus_initialize(1);
  
  (void)SPI_LOCK(spi, true);
  SPI_SETMODE(spi, SPIDEV_MODE2);
  SPI_SETBITS(spi, 8);
  (void)SPI_HWFEATURES(spi, 0);
  printf("SPI wanted freq: 2000000, actual freq:%d\n",SPI_SETFREQUENCY(spi, 2000000));

  stm32_configgpio(GPIO_RESET_DDS);
  stm32_configgpio(GPIO_SLEEP_DDS);
  stm32_configgpio(GPIO_DET);

  stm32_gpiowrite(GPIO_SLEEP_DDS, false); // lo
  stm32_gpiowrite(GPIO_DET, true);        // hi
  stm32_gpiowrite(GPIO_RESET_DDS, true);  // hi
  usleep(100);
  stm32_gpiowrite(GPIO_RESET_DDS, false); // lo

  envoi_DDS(ctrl_r); // must send LSB first, then MSB
  usleep(15000);
  envoi_DDS(ctrl_nr); // must send LSB first, then MSB
  envoi_DDS(phase0);
  envoi_DDS(phase1);
  return(0);
}

void envoi_DDS (unsigned short entree)  // 16 bit sent
{ SPI_SELECT(spi, SPIDEV_USER(0), true); // which component on the SPI bus to select
  // arch/arm/src/stm32/stm32_spi.c:  .select            = stm32_spi1select,
  SPI_SEND(spi, (entree>>8)&0xff);
  SPI_SEND(spi, (entree)&0xff);
  //printf("DDS: %x\n",entree);
  // usleep(100);                           // a revoir
  SPI_SELECT(spi, SPIDEV_USER(0), false);
}

static int dds_close(file_t *filep)
{ printf("Close\n");
  (void)SPI_LOCK(spi, false);
  return OK;
}

static long frequency;  // local shared variable between read and write

static ssize_t dds_read(file_t *filep, FAR char *buf, size_t buflen)
{int k;
 if (buflen!=4) buflen=4;
 for (k=0;k<buflen;k++) {buf[k]=(frequency >> (k*4))&0xff;}
 printf("Read %d\n",buflen);
 return buflen;
}

static ssize_t dds_write(file_t *filep, FAR const char *buf, size_t buflen)
{unsigned long frequence=*(long*)buf;
 // printf("DDS frequency=%x\n",frequence);
 envoi_DDS((frequence&0x3fff)|0x4000);       // LSB
 envoi_DDS(((frequence>>14)&0x3fff)|0x4000); // MSB

// envoi_DDS((frequence&0x3fff)|0x8000);       // LSB
// envoi_DDS(((frequence>>14)&0x3fff)|0x8000); // MSB
 return buflen;
}

static int dds_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{switch (cmd)
    {case 0:
        {printf("RESET DDS: %d\n",arg); // TODO : DDS_RESET
        }
     break;
     case 1:
        {printf("another ioctl ...\n");
        }
     break;
     default:
       return -1;
    }
 return OK; 
}
/****************************************************************************
 * Initialize device, add /dev/... nodes
 ****************************************************************************/
void stm32_dds_setup(void)
{
  (void)register_driver("/dev/dds", &dds_ops, 0444, NULL);
}

/*
no SPI driver -- SPI is a bus
http://nuttx.org/doku.php?id=wiki:nxinternal:devices-vs-buses

nsh> echo "ab" > /dev/dds
Open
JMF: SPI1
spi_setmode: mode=0
spi_setbits: nbits=8
stm32_spi1select: devid: 1245184 CS: assert
Write 4
spi_send: Sent: 0061 Return: 0000 Status: 02
spi_send: Sent: 0062 Return: 0000 Status: 02
spi_send: Sent: 0020 Return: 0000 Status: 02
spi_send: Sent: 000a Return: 0000 Status: 02
stm32_spi1select: devid: 1245184 CS: de-assert
Close
*/
