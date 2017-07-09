#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <string.h>

#include <nuttx/drivers/pwm.h>
#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>

#define NB_MOY 16

struct network_analyzer {long fstart;long fstop;int nb_pts;};

static void network_help(void)
{ printf("Usage: network [OPTIONS]\n");
  printf("  [-h] shows this message and exits\n");
  printf("  [-a] start frequency\n");
  printf("  [-b] stop frequency\n");
  printf("  [-n] number of steps\n");
}

static int arg_string(FAR char **arg, FAR char **value)
{ FAR char *ptr = *arg;

  if (ptr[2] == '\0') {*value = arg[1];return 2;}
  else {*value = &ptr[2];return 1;}
}
static int network_decimal(FAR char **arg, FAR long *value)
{ FAR char *string;
  int ret;
  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}

static void parse_args(FAR struct network_analyzer *n, int argc, FAR char **argv)
{
  FAR char *ptr;
  long value;
  int index;
  int nargs;

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          printf("Invalid options format: %s\n", ptr);
          exit(0);
        }

      switch (ptr[1])
        {
          case 'a':
            nargs = network_decimal(&argv[index], &value);
            if (value < 0)
              {
                printf("Count must be non-negative: %ld\n", value);
                exit(1);
              }

            n->fstart= (uint32_t)value;
            index += nargs;
            break;
         
          case 'b':
            nargs = network_decimal(&argv[index], &value);
            if (value < 0)
              {
                printf("Count must be non-negative: %ld\n", value);
                exit(1);
              }

            n->fstop= (uint32_t)value;
            index += nargs;
            break;

          case 'n':
            nargs = network_decimal(&argv[index], &value);
            if (value < 0)
              {
                printf("Count must be non-negative: %ld\n", value);
                exit(1);
              }

            n->nb_pts= (uint32_t)value;
            index += nargs;
            break;
          case 'h':
            network_help();
            exit(0);

          default:
            printf("Unsupported option: %s\n", ptr);
            network_help();
            exit(1);
        }
    }
}

void lit_adc(int fd,int *moy1,int *moy2)
{int j,ret,nsamples;
 size_t readsize;
 ssize_t nbytes;
 struct adc_msg_s sample[CONFIG_EXAMPLES_ADC_GROUPSIZE];
 *moy1=0;
 *moy2=0;
 for (j=0;j<NB_MOY;j++)
     {ret = ioctl(fd, ANIOC_TRIGGER, 0);
      if (ret < 0)
         printf("adc_main: ANIOC_TRIGGER ioctl failed\n");
      readsize = CONFIG_EXAMPLES_ADC_GROUPSIZE * sizeof(struct adc_msg_s);
      nbytes = read(fd, sample, readsize);
      nsamples = nbytes / sizeof(struct adc_msg_s);
      if (nsamples * sizeof(struct adc_msg_s) != nbytes)
         printf("adc_main: read size=%ld is not a multiple of sample size=%d, Ignoring\n",
                   (long)nbytes, sizeof(struct adc_msg_s));
      else {//printf("ADC: "); 
            *moy1+=sample[0].am_data;
            *moy2+=sample[1].am_data;
            //printf("ch=%d val=%d\n", sample[i].am_channel, sample[i].am_data);
           }
     }
 *moy1/=NB_MOY;
 *moy2/=NB_MOY;
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int network_main(int argc, char *argv[])
#endif
{ struct network_analyzer my_network;
  int duty=50;   // on convertira cette valeur en % vers une valeur 16 bits
  struct pwm_info_s info;
  int fd_pwm,fd_dds,fd_adc0;
  int ret;
  int moy1,moy2;
  long fstep;
  long freq=0;
  my_network.fstart=0x2869536-0x5D9F7/8; // 11.05 MHz  /16
  my_network.fstop =0x2869536+0x5D9F7/8; // 11.05 MHz   /8
  my_network.nb_pts=1024;
  fstep=(my_network.fstop-my_network.fstart)/my_network.nb_pts;

  printf("Network analyzer application\n");
  parse_args(&my_network, argc, argv);

  fd_pwm=open("/dev/pwm0",O_RDONLY);
  info.frequency = 70000000;  // 70 MHz
  info.duty      = ((uint32_t)duty << 16) / 100; // (%/100)*2^16
  ret = ioctl(fd_pwm, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&info));
  if (ret < 0)
    {printf("pwm_main: ioctl(PWMIOC_SETCHARACTERISTICS) failed: %d\n", errno);
     goto errout_with_dev;
    }
  /* Since the driver was opened in blocking mode, this call will block if the */
  /* count value is greater than zero. */
  ret = ioctl(fd_pwm, PWMIOC_START, 0);
  if (ret < 0)
    {printf("pwm_main: ioctl(PWMIOC_START) failed: %d\n", errno);
     goto errout_with_dev;
    }

  fd_dds=open("/dev/dds",O_RDWR);    // open = init DDS
  fd_adc0=open("/dev/adc0",O_RDONLY);
  
  for (freq=my_network.fstart;freq<my_network.fstop;freq+=fstep)
   {write(fd_dds,&freq,sizeof(freq)); // fstart+=fstep;
    printf("0x%x ",freq);
    usleep(10000);
// envoy\'e message dans /dev/dds
// lire dans ADC tel que propose' dans examples/adc
   lit_adc(fd_adc0,&moy1,&moy2);printf("%d %d\n",moy1,moy2);
  } 

  ret = ioctl(fd_pwm, PWMIOC_STOP, 0);
  if (ret < 0)
    {printf("pwm_main: ioctl(PWMIOC_STOP) failed: %d\n", errno);
     goto errout_with_dev;
    }

  close(fd_pwm);
  close(fd_dds);
  close(fd_adc0);
  fflush(stdout);
  return OK;

errout_with_dev:
  close(fd_pwm);
  fflush(stdout);
  return ERROR;
}


