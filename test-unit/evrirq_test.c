/*
  evrirq.c --   Micro-Research Event Receiver
                Pulse interrupt example

  Author: Jukka Pietarinen (MRF)
  Date:   10.11.2023

*/

#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include "erapi.h"
#include <time.h> 

struct MrfErRegs *pEr;
int              fdEr;

struct timespec tic, toc;

void evr_irq_handler(int param)
{
  int flags, i;
  struct FIFOEvent fe;

  flags = EvrGetIrqFlags(pEr);

  if (flags & (1 << C_EVR_IRQFLAG_PULSE))
    {
      clock_gettime(CLOCK_MONOTONIC, &tic); 
      print_debug_info("%.5f\n",(double)tic.tv_sec + 1.0e-9 * tic.tv_nsec);
      printf("Pulse IRQ\n");
      EvrClearIrqFlags(pEr, (1 << C_EVR_IRQFLAG_PULSE));
    }
  
  EvrIrqHandled(fdEr);
}
  
int irqsetup(volatile struct MrfErRegs *pEr)
{
  int i;

  EvrEnable(pEr, 1);
  if (!EvrGetEnable(pEr))
    {
      printf("Could not enable EVR!\n");
      return -1;
    }
  
  EvrSetTargetDelay(pEr, 0x01000000);
  EvrDCEnable(pEr, 1);
  
  EvrGetViolation(pEr, 1);

  /* Build configuration for EVR map RAMS */

  EvrSetLedEvent(pEr, 0, 0x02, 1);
  /* Map event 0x01 to trigger pulse generator 0 */
  EvrSetPulseMap(pEr, 0, 0x02, 0, -1, -1);
  EvrSetPulseParams(pEr, 0, 1, 100, 100);
  EvrSetPulseProperties(pEr, 0, 0, 0, 0, 1, 1);
  EvrSetUnivOutMap(pEr, 0, 0x3f00);
  EvrSetPulseIrqMap(pEr, 0x3f00);

  EvrSetExtEvent(pEr, 0, 0x01, 1, 0);

  EvrMapRamEnable(pEr, 0, 1);

  EvrOutputEnable(pEr, 1);

  EvrIrqAssignHandler(pEr, fdEr, &evr_irq_handler);
  EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_PULSE);

  while(1);
}

int main(int argc, char *argv[])
{
  if (argc < 2)
    {
      printf("Usage: %s /dev/er3a3\n", argv[0]);
      printf("Assuming: /dev/er3a3\n");
      argv[1] = "/dev/er3a3";
    }

  fdEr = EvrOpen(&pEr, argv[1]);
  
  if (fdEr == -1)
    {
      return errno;
    }

  irqsetup(pEr);

  EvrClose(fdEr);
}