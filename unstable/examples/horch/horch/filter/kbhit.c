/********************************************************************
* filename: kbhit.c                                                 *
* purpose : simulate the kbhit() and getch() functions              *
*           known from DOS                                          *
* Author  : Christian Schoett                                       *
* Date    : 20.02.2001                                              *
* Version : 1.0                                                     *
********************************************************************/

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "kbhit.h"

static struct termios orig, tnew;
static int peek = -1;
static int oldflags;

void kbdinit(void)
{
//  oldflags = fcntl(STDIN_FILENO,F_GETFL);
//  printf("flags %x\n",oldflags);
//  fcntl(STDIN_FILENO,F_SETFL,O_NONBLOCK);
  tcgetattr(STDIN_FILENO, &orig);
  tnew = orig;
  tnew.c_lflag &= ~ICANON;
//  tnew.c_lflag &= ~ECHO;
//  tnew.c_lflag &= ~ISIG;
  tnew.c_cc[VMIN] = 1;
  tnew.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &tnew);
}

void kbdexit(void)
{
  tcsetattr(STDIN_FILENO,TCSANOW, &orig);
//  fcntl(STDIN_FILENO,F_SETFL,oldflags);
}

// No longer used

int kbhit(void)
{
  char ch;
  int nread = -1;

  fflush(stdout);
  if(peek != -1) return 1;
  tnew.c_cc[VMIN]=0;
  tcsetattr(STDIN_FILENO, TCSANOW, &tnew);
  nread = read(STDIN_FILENO,&ch,1);
  tnew.c_cc[VMIN]=1;
  tcsetattr(STDIN_FILENO, TCSANOW, &tnew);

  if(nread == 1) {
   /* printf("Read %2.2x\n",ch); */
   peek = ch;
   return nread;
  }

  return 0;
}

int readch(void)
{
  char ch;

  if(peek != -1) {
    ch = peek;
    peek = -1;
    return ch;
  }

  read(0,&ch,1);
  return ch;
}

