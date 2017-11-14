#include "signal_handlers.h"
#include <signal.h>
void catch_sigint(int signalNo)
{
  // TODO: File this!
 // signal(signalNo,SIG_IGN);
 // signal(SIGINT,catch_sigint);
  signal(SIGINT,SIG_IGN);
}

void catch_sigtstp(int signalNo)
{
  // TODO: File this!
 // signal(signalNo,SIG_IGN);
 // signal(SIGTSTP,catch_sigtstp);
  signal(SIGTSTP,SIG_IGN);
}
