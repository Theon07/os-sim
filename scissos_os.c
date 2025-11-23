#include "ScisSos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void set_scheduling_algorithm(int algorithm) {
    if(algorithm >= SCHED_RR && algorithm <= SCHED_SJF) {
        _scheduling_algorithm = algorithm;
        printf("Scheduling algorithm set to: %s\n", get_sched_algorithm_name(algorithm));
    } else {
        printf("Invalid scheduling algorithm\n");
    }
}

int schedule_rr(void) {
    if(_ready_count == 0) return EMPTY;
    
    int selected_pid = _readyQ[0];
    
    for(int i = 0; i < _ready_count - 1; i++) {
        _readyQ[i] = _readyQ[i + 1];
    }
    _ready_count--;
    
    return selected_pid;
}

int schedule_fcfs(void) {
    if(_ready_count == 0) return EMPTY;
    
    int earliest_arrival = _current_time + 1000;
    int selected_pid = EMPTY;
    
    for(int i = 0; i < _ready_count; i++) {
        int pid = _readyQ[i];
        if(_proctable[pid] && _proctable[pid]->arrival_time < earliest_arrival) {
            earliest_arrival = _proctable[pid]->arrival_time;
            selected_pid = pid;
        }
    }
    
    if(selected_pid != EMPTY) {
        for(int i = 0; i < _ready_count; i++) {
            if(_readyQ[i] == selected_pid) {
                for(int j = i; j < _ready_count - 1; j++) {
                    _readyQ[j] = _readyQ[j + 1];
                }
                _ready_count--;
                break;
            }
        }
    }
    
    return selected_pid;
}

int schedule_priority(void) {
    if(_ready_count == 0) return EMPTY;
    
    int highest_priority = -1;
    int selected_pid = EMPTY;
    
    for(int i = 0; i < _ready_count; i++) {
        int pid = _readyQ[i];
        if(_proctable[pid] && _proctable[pid]->priority_value > highest_priority) {
            highest_priority = _proctable[pid]->priority_value;
            selected_pid = pid;
        }
    }
    
    if(selected_pid != EMPTY) {
        for(int i = 0; i < _ready_count; i++) {
            if(_readyQ[i] == selected_pid) {
                for(int j = i; j < _ready_count - 1; j++) {
                    _readyQ[j] = _readyQ[j + 1];
                }
                _ready_count--;
                break;
            }
        }
    }
    
    return selected_pid;
}

int schedule_sjf(void) {
    if(_ready_count == 0) return EMPTY;
    
    int shortest_job = 1000000;
    int selected_pid = EMPTY;
    
    for(int i = 0; i < _ready_count; i++) {
        int pid = _readyQ[i];
        if(_proctable[pid] && _proctable[pid]->remaining_time < shortest_job) {
            shortest_job = _proctable[pid]->remaining_time;
            selected_pid = pid;
        }
    }
    
    if(selected_pid != EMPTY) {
        for(int i = 0; i < _ready_count; i++) {
            if(_readyQ[i] == selected_pid) {
                for(int j = i; j < _ready_count - 1; j++) {
                    _readyQ[j] = _readyQ[j + 1];
                }
                _ready_count--;
                break;
            }
        }
    }
    
    return selected_pid;
}

int schedule_process(void) {
    switch(_scheduling_algorithm) {
        case SCHED_RR:
            return schedule_rr();
        case SCHED_FCFS:
            return schedule_fcfs();
        case SCHED_PRIORITY:
            return schedule_priority();
        case SCHED_SJF:
            return schedule_sjf();
        default:
            return schedule_rr();
    }
}

void scissos_initialise(void) {
    printf("Initializing ScisSOS...\n");
    
    for(int i = 0; i < MAXPROC; i++) {
        _proctable[i] = NULL;
    }
    
    for(int i = 0; i < MAXPROC; i++) {
        _readyQ[i] = EMPTY;
        _blockQ[i] = EMPTY;
    }
    
    _ready_count = 0;
    _block_count = 0;
    _current_time = 0;
    _next_pid = 0;
    _current_running_pid = EMPTY;
    _scheduling_algorithm = SCHED_RR;
    
    srand(time(NULL));
    printf("ScisSOS initialized successfully.\n");
}

void update_queues(void) {
    for(int i = 0; i < MAXPROC; i++) {
        if(_proctable[i] != NULL && _proctable[i]->ps_state == PS_NEW) {
            _proctable[i]->ps_state = PS_RDY;
            _readyQ[_ready_count++] = i;
        }
    }
    
    for(int i = 0; i < _block_count; i++) {
        int pid = _blockQ[i];
        if(_proctable[pid] && _proctable[pid]->ps_state == PS_BLK) {
            if(rand() % 100 < 30) {
                _proctable[pid]->ps_state = PS_RDY;
                
                for(int j = i; j < _block_count - 1; j++) {
                    _blockQ[j] = _blockQ[j + 1];
                }
                _block_count--;
                
                _readyQ[_ready_count++] = pid;
            }
        }
    }
}

void scissos_call_scheduler(void) {
    printf("\n=== Scheduler (Time: %d, Algorithm: %s) ===\n", 
           _current_time, get_sched_algorithm_name(_scheduling_algorithm));
    
    update_queues();
    
    int active_processes = 0;
    for(int i = 0; i < MAXPROC; i++) {
        if(_proctable[i] != NULL && _proctable[i]->ps_state != PS_DEAD) {
            active_processes++;
        }
    }
    
    if(active_processes == 0) {
        printf("No active processes. Simulation complete.\n");
        return;
    }
    
    if(_current_running_pid != EMPTY && _proctable[_current_running_pid] != NULL) {
        if(_proctable[_current_running_pid]->ps_state == PS_RUN) {
            _proctable[_current_running_pid]->ps_state = PS_RDY;
            _readyQ[_ready_count++] = _current_running_pid;
        }
    }
    
    int next_pid = schedule_process();
    
    if(next_pid == EMPTY) {
        printf("No processes in ready queue.\n");
        _current_running_pid = EMPTY;
    } else {
        _proctable[next_pid]->ps_state = PS_RUN;
        _current_running_pid = next_pid;
        printf("Scheduled process %d to run\n", next_pid);
        
        int result = scissos_proc_run(next_pid);
        printf("\n");
        
        if(result == 1) {
            printf("Process %d blocked on long system call\n", next_pid);
        } else if(result == 0 && _proctable[next_pid]->ps_state == PS_DEAD) {
            printf("Process %d terminated\n", next_pid);
        }
    }
    
    _current_time++;
}

void print_system_status(void) {
    printf("\n=== System Status (Time: %d) ===\n", _current_time);
    printf("Scheduling Algorithm: %s\n", get_sched_algorithm_name(_scheduling_algorithm));
    printf("Ready Queue (%d): ", _ready_count);
    for(int i = 0; i < _ready_count; i++) {
        printf("%d ", _readyQ[i]);
    }
    printf("\n");
    
    printf("Blocked Queue (%d): ", _block_count);
    for(int i = 0; i < _block_count; i++) {
        printf("%d ", _blockQ[i]);
    }
    printf("\n");
    
    printf("Running: %d\n", _current_running_pid);
    
    printf("\nProcess Table:\n");
    printf("PID\tState\t\tType\t\t\tPC\tRemain\tPriority\n");
    for(int i = 0; i < MAXPROC; i++) {
        if(_proctable[i] != NULL) {
            printf("%d\t%-12s\t%-20s\t%d\t%d\t%d\n",
                   i,
                   get_state_name(_proctable[i]->ps_state),
                   get_type_name(_proctable[i]->p_type),
                   _proctable[i]->pc,
                   _proctable[i]->remaining_time,
                   _proctable[i]->priority_value);
        }
    }
}

int main() {
    scissos_initialise();
    
    printf("\n=== Creating Processes ===\n");
    ScisSosProcess *p1 = scissos_proc_create("Process 1", 1, 50, PT_REG);
    p1->_pcb->priority_value = 10;
    
    ScisSosProcess *p2 = scissos_proc_create("Process 2", 1, 100, PT_CMP);
    p2->_pcb->priority_value = 5;
    
    ScisSosProcess *p3 = scissos_proc_create("Process 3", 2, 80, PT_IOE);
    p3->_pcb->priority_value = 15;
    
    ScisSosProcess *p4 = scissos_proc_create("Process 4", 2, 120, PT_IOE);
    p4->_pcb->priority_value = 20;
    
    printf("\n=== Testing Different Scheduling Algorithms ===\n");
    
    set_scheduling_algorithm(SCHED_RR);
    for(int i = 0; i < 5; i++) {
        scissos_call_scheduler();
    }
    
    set_scheduling_algorithm(SCHED_FCFS);
    for(int i = 0; i < 5; i++) {
        scissos_call_scheduler();
    }
    
    set_scheduling_algorithm(SCHED_PRIORITY);
    for(int i = 0; i < 5; i++) {
        scissos_call_scheduler();
    }
    
    set_scheduling_algorithm(SCHED_SJF);
    for(int i = 0; i < 10; i++) {
        scissos_call_scheduler();
        
        int all_done = 1;
        for(int j = 0; j < MAXPROC; j++) {
            if(_proctable[j] != NULL && _proctable[j]->ps_state != PS_DEAD) {
                all_done = 0;
                break;
            }
        }
        if(all_done) {
            printf("\nAll processes completed.\n");
            break;
        }
    }
    
    printf("\n=== Final Status ===\n");
    print_system_status();
    
    return 0;
}