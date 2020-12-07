/* include/linux/kernel.h */

#ifndef _KERNEL_H
#define _KERNEL_H

extern volatile void panic(const char * str);

#define suser() (current->euid == 0)  // check if super user

#endif
