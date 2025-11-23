#ifndef SCISSOS_H
#define SCISSOS_H

#include <stdio.h>

#define MAXPROC     1000
#define MAXUSRS     10
#define DEFPRIO     20
#define EMPTY       -100
#define MAXPGES     10
#define DEFTS       5
#define REG_THR     0.02
#define CMP_THR     0.001
#define IOE_THR     0.2

#define PS_NEW  0
#define PS_RDY  1
#define PS_RUN  2
#define PS_BLK  3
#define PS_SRDY 4
#define PS_SBLK 5
#define PS_DEAD 6

#define PT_REG  0
#define PT_CMP  1
#define PT_IOE  2
#define MT_GOOD 3
#define MT_BAD  4
#define MT_UGLY 5
#define INS_LNG 10
#define INS_SHR 20

#define SCHED_RR 0
#define SCHED_FCFS 1
#define SCHED_PRIORITY 2
#define SCHED_SJF 3

typedef int ScisSosPGTable[2];

typedef struct {
     int _inum;
     int _syscall;
     int _addref;
} ScisSosInst;

typedef struct {
     int pid;
     int uid;
     int size;
     int priority_value;
     int ps_state;
     int p_type;
     int m_type;
     int pc;
     ScisSosInst **p_code;
     ScisSosPGTable pg_table[MAXPGES];
     int p_timeslice;
     int arrival_time;
     int remaining_time;
} ScisSosPCB;

typedef struct {
     char _pname[80];
     int _PID;
     int _psize;
     ScisSosPCB *_pcb;
     ScisSosInst **_CODE;
} ScisSosProcess;

extern ScisSosPCB *_proctable[MAXPROC];
extern int _readyQ[MAXPROC];
extern int _blockQ[MAXPROC];
extern int _ready_count;
extern int _block_count;
extern int _current_time;
extern int _next_pid;
extern int _current_running_pid;
extern int _scheduling_algorithm;

const char* get_type_name(int type);
const char* get_state_name(int state);
const char* get_sched_algorithm_name(int algorithm);

ScisSosProcess *scissos_proc_create(char *, int, int, int);
int scissos_proc_save(ScisSosProcess *, FILE *);
void scissos_print_pcb(ScisSosProcess *, FILE *);
int scissos_proc_run(int);

void scissos_initialise(void);
void scissos_call_scheduler(void);
void print_system_status(void);
void set_scheduling_algorithm(int algorithm);

int schedule_rr(void);
int schedule_fcfs(void);
int schedule_priority(void);
int schedule_sjf(void);

#endif