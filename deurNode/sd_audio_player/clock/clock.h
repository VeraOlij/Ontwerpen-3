#ifndef CLOCK_H_
#define CLOCK_H_

void Config32MHzClock(void);
void Config32MHzClock_Ext16M(void);
void Config16MHzClock_Ext16M(void);

void inline init_clock(void) {
  Config32MHzClock_Ext16M();
}

#endif // CLOCK_H_ 