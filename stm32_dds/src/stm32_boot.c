#include <nuttx/config.h>
#include "up_arch.h"

#include <debug.h>
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/spi/spi.h>
#include <nuttx/drivers/pwm.h>
#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <nuttx/fs/ioctl.h>
#include <../stm32/stm32_pwm.h> // JMF : retire warnings concernant les fonctions init PWM 

#include <stdio.h>

#include "stm32_dds.h"

void stm32_dummy_setup(void);

void stm32_boardinitialize(void)
{
}

#ifdef CONFIG_BOARD_INITIALIZE
void board_initialize(void)
{
  struct pwm_lowerhalf_s *pwm;
  struct pwm_info_s info;
  /* Perform NSH initialization here instead of from the NSH.  This
   * alternative NSH initialization is necessary when NSH is ran in user-space
   * but the initialization function must run in kernel space.
   */

#if defined(CONFIG_NSH_LIBRARY) && !defined(CONFIG_LIB_BOARDCTL)
  board_app_initialize(0);
#endif

  stm32_adc_setup();          // jmf adc 
  stm32_spidev_initialize();  // jmf spi
  stm32_dummy_setup();        // dummy driver read/write
  stm32_dds_setup();          // SPI DDS driver 

  pwm=stm32_pwminitialize(1); // timer1/output1
  if (!pwm)
     lederr("ERROR: Failed to get the STM32 PWM lower half\n");
  else 
    {
     pwm_register("/dev/pwm0", pwm);
     info.frequency = 100000;
     info.duty = (50<<16);
     pwm->ops->setup(pwm);
     pwm->ops->start(pwm, &info);
// cf apps/examples/pwm for userspace call to the PWM driver
     syslog(LOG_INFO,"JMF Boot: PWM init completed\n");
    }
  syslog(LOG_INFO,"JMF Boot: DDS init completed\n");
}
#endif

// cf configs/stm32f103-minimum/src/stm32_rgbled.c 
// tested with 
// nsh> pwm -f 1000000 -t 20 -d 50 on the signal clocking the DDS
