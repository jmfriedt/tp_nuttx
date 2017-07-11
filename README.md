# tp_nuttx
NuttX application and platform examples

Symbolic link of demo_thread_jmf and network to the apps/ directory: in the apps/ dir, 

    ln -s ../tp_nuttx/network . && ln -s ../tp_nuttx/demo_thread_jmf .

Symbolic link of stm32_dds to the nuttx/configs/ directory: in the nuttx/configs directory, 

    ln -s ../../tp_nuttx/stm32_dds . 

Add the platform to the configs/Kconfig description

    config ARCH_BOARD_STM32_DDS                                                                  
      bool "STM32 DDS"                                                                      
      depends on ARCH_CHIP_STM32F410RB                                                      
      ---help---                                                                            
      This is a minimal configuration that supports low-level test of the           
      Nucleo F410RB in the NuttX source tree.  

and

    default "stm32_dds"                if ARCH_BOARD_STM32_DDS

and

    if ARCH_BOARD_STM32_DDS                                                                      
       source "configs/stm32_dds/Kconfig"                                                           
    endif
