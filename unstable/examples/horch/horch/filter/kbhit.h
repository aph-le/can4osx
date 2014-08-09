/********************************************************************
* filename: kbhit.h                                                 *
* purpose : simulate the kbhit() and getch() functions              *
*           known from DOS                                          *
* Author  : Christian Schoett                                       *
* Date    : 20.02.2001                                              *
* Version : 1.0                                                     *
********************************************************************/

#ifndef _KBHIT_H
#define _KBHIT_H

/* function prototypes */
void kbdinit(void);
void kbdexit(void);
int kbhit(void);
int readch(void);

#endif

