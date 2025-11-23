#include "ScisSos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

ScisSosPCB *_proctable[MAXPROC] = {NULL};
int _readyQ[MAXPROC];
int _blockQ[MAXPROC];
int _ready_count = 0;
int _block_count = 0;
int _current_time = 0;
int _next_pid = 0;
int _current_running_pid = EMPTY;
int _scheduling_algorithm = SCHED_RR;

const char* get_type_name(int type) {
    switch(type) {
        case PT_REG: return "REGULAR";
        case PT_CMP: return "COMPUTE_INTENSIVE";
        case PT_IOE: return "IO_INTENSIVE";
        default: return "UNKNOWN";
    }
}

const char* get_state_name(int state) {
    switch(state) {
        case PS_NEW: return "NEW";
        case PS_RDY: return "READY";
        case PS_RUN: return "RUNNING";
        case PS_BLK: return "BLOCKED";
        case PS_SRDY: return "SUSPENDED_READY";
        case PS_SBLK: return "SUSPENDED_BLOCKED";
        case PS_DEAD: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

const char* get_sched_algorithm_name(int algorithm) {
    switch(algorithm) {
        case SCHED_RR: return "ROUND_ROBIN";
        case SCHED_FCFS: return "FIRST_COME_FIRST_SERVED";
        case SCHED_PRIORITY: return "PRIORITY";
        case SCHED_SJF: return "SHORTEST_JOB_FIRST";
        default: return "UNKNOWN";
    }
}

void generate_instructions(ScisSosProcess *process) {
    double long_threshold;
    
    switch(process->_pcb->p_type) {
        case PT_CMP:
            long_threshold = CMP_THR;
            break;
        case PT_IOE:
            long_threshold = IOE_THR;
            break;
        case PT_REG:
        default:
            long_threshold = REG_THR;
            break;
    }
    
    process->_CODE = (ScisSosInst**)malloc(process->_psize * sizeof(ScisSosInst*));
    process->_pcb->p_code = process->_CODE;
    
    for(int i = 0; i < process->_psize; i++) {
        process->_CODE[i] = (ScisSosInst*)malloc(sizeof(ScisSosInst));
        process->_CODE[i]->_inum = i;
        
        double rand_val = (double)rand() / RAND_MAX;
        if(rand_val < long_threshold) {
            process->_CODE[i]->_syscall = INS_LNG;
        } else {
            process->_CODE[i]->_syscall = INS_SHR;
        }
        
        switch(process->_pcb->m_type) {
            case MT_GOOD:
                process->_CODE[i]->_addref = i;
                break;
            case MT_BAD:
                process->_CODE[i]->_addref = rand() % process->_psize;
                break;
            case MT_UGLY:
                if(i % 4 == 0) {
                    process->_CODE[i]->_addref = rand() % process->_psize;
                } else {
                    process->_CODE[i]->_addref = i;
                }
                break;
            default:
                process->_CODE[i]->_addref = i;
        }
    }
}

void initialize_page_table(ScisSosPCB *pcb) {
    for(int i = 0; i < MAXPGES; i++) {
        pcb->pg_table[i][0] = i;
        pcb->pg_table[i][1] = EMPTY;
    }
}

ScisSosProcess *scissos_proc_create(char *pname, int uid, int size, int ptype) {
    if(_next_pid >= MAXPROC) {
        printf("Error: Maximum number of processes (%d) reached!\n", MAXPROC);
        return NULL;
    }
    
    ScisSosProcess *new_process = (ScisSosProcess*)malloc(sizeof(ScisSosProcess));
    
    strncpy(new_process->_pname, pname, 79);
    new_process->_pname[79] = '\0';
    
    new_process->_PID = _next_pid;
    new_process->_psize = size;
    
    new_process->_pcb = (ScisSosPCB*)malloc(sizeof(ScisSosPCB));
    new_process->_pcb->pid = _next_pid;
    new_process->_pcb->uid = uid;
    new_process->_pcb->size = size;
    new_process->_pcb->priority_value = DEFPRIO;
    new_process->_pcb->ps_state = PS_NEW;
    new_process->_pcb->p_type = ptype;
    new_process->_pcb->m_type = MT_GOOD;
    new_process->_pcb->pc = 0;
    new_process->_pcb->p_timeslice = DEFTS;
    new_process->_pcb->arrival_time = _current_time;
    new_process->_pcb->remaining_time = size;
    
    initialize_page_table(new_process->_pcb);
    generate_instructions(new_process);
    
    _proctable[_next_pid] = new_process->_pcb;
    
    printf("Created process %s (PID: %d, UID: %d, Size: %d, Type: %s)\n",
           pname, _next_pid, uid, size, get_type_name(ptype));
    
    _next_pid++;
    return new_process;
}

int scissos_proc_run(int pid) {
    if(pid < 0 || pid >= MAXPROC || _proctable[pid] == NULL) {
        return -1;
    }
    
    ScisSosPCB *pcb = _proctable[pid];
    if(pcb->ps_state != PS_RUN) {
        return -2;
    }
    
    printf("Running process PID %d (%s): ", pid, get_type_name(pcb->p_type));
    int instructions_executed = 0;
    
    while(pcb->pc < pcb->size && instructions_executed < pcb->p_timeslice) {
        ScisSosInst *current_inst = pcb->p_code[pcb->pc];
        
        printf("%d(%s) ", pcb->pc, 
               current_inst->_syscall == INS_LNG ? "L" : "S");
        
        if(current_inst->_syscall == INS_LNG) {
            printf("[LONG SYSCALL - BLOCKING] ");
            pcb->ps_state = PS_BLK;
            _blockQ[_block_count++] = pid;
            pcb->pc++;
            pcb->remaining_time--;
            return 1;
        }
        
        pcb->pc++;
        pcb->remaining_time--;
        instructions_executed++;
        
        if(pcb->pc >= pcb->size) {
            pcb->ps_state = PS_DEAD;
            printf("[COMPLETED] ");
            return 0;
        }
    }
    
    if(pcb->pc < pcb->size) {
        pcb->ps_state = PS_RDY;
        _readyQ[_ready_count++] = pid;
        printf("[TIME SLICE EXPIRED] ");
    }
    
    return 0;
}

int scissos_proc_save(ScisSosProcess *process, FILE *file) {
    if(!process || !file) return -1;
    
    fprintf(file, "Process Name: %s\n", process->_pname);
    fprintf(file, "PID: %d\n", process->_PID);
    fprintf(file, "Size: %d\n", process->_psize);
    fprintf(file, "State: %d\n", process->_pcb->ps_state);
    fprintf(file, "Program Counter: %d\n", process->_pcb->pc);
    
    return 0;
}

void scissos_print_pcb(ScisSosProcess *process, FILE *file) {
    if(!process || !file) return;
    
    ScisSosPCB *pcb = process->_pcb;
    
    fprintf(file, "=== PCB for Process %s (PID: %d) ===\n", 
            process->_pname, process->_PID);
    fprintf(file, "User ID: %d\n", pcb->uid);
    fprintf(file, "Size: %d instructions\n", pcb->size);
    fprintf(file, "Priority: %d\n", pcb->priority_value);
    fprintf(file, "State: %s\n", get_state_name(pcb->ps_state));
    fprintf(file, "Type: %s\n", get_type_name(pcb->p_type));
    fprintf(file, "Memory Behavior: %d\n", pcb->m_type);
    fprintf(file, "Program Counter: %d\n", pcb->pc);
    fprintf(file, "Time Slice: %d\n", pcb->p_timeslice);
    fprintf(file, "Remaining Time: %d\n", pcb->remaining_time);
    
    fprintf(file, "Page Table:\n");
    for(int i = 0; i < MAXPGES; i++) {
        if(pcb->pg_table[i][1] != EMPTY) {
            fprintf(file, "  Page %d -> Frame %d\n", 
                    pcb->pg_table[i][0], pcb->pg_table[i][1]);
        }
    }
    
    fprintf(file, "First 10 Instructions:\n");
    for(int i = 0; i < 10 && i < pcb->size; i++) {
        fprintf(file, "  %d: Syscall=%s, AddrRef=%d\n",
                pcb->p_code[i]->_inum,
                pcb->p_code[i]->_syscall == INS_LNG ? "LONG" : "SHORT",
                pcb->p_code[i]->_addref);
    }
}