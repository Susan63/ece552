
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"
#include "decode.def"

#include "instr.h"

/* PARAMETERS OF THE TOMASULO'S ALGORITHM */

#define INSTR_QUEUE_SIZE         10

#define RESERV_INT_SIZE    4
#define RESERV_FP_SIZE     2
#define FU_INT_SIZE        2
#define FU_FP_SIZE         1

#define FU_INT_LATENCY     4
#define FU_FP_LATENCY      9

/* IDENTIFYING INSTRUCTIONS */

//unconditional branch, jump or call
#define IS_UNCOND_CTRL(op) (MD_OP_FLAGS(op) & F_CALL || \
                         MD_OP_FLAGS(op) & F_UNCOND)

//conditional branch instruction
#define IS_COND_CTRL(op) (MD_OP_FLAGS(op) & F_COND)

//floating-point computation
#define IS_FCOMP(op) (MD_OP_FLAGS(op) & F_FCOMP)

//integer computation
#define IS_ICOMP(op) (MD_OP_FLAGS(op) & F_ICOMP)

//load instruction
#define IS_LOAD(op)  (MD_OP_FLAGS(op) & F_LOAD)

//store instruction
#define IS_STORE(op) (MD_OP_FLAGS(op) & F_STORE)

//trap instruction
#define IS_TRAP(op) (MD_OP_FLAGS(op) & F_TRAP) 

#define USES_INT_FU(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_STORE(op))
#define USES_FP_FU(op) (IS_FCOMP(op))

#define WRITES_CDB(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_FCOMP(op))

/* FOR DEBUGGING */

//prints info about an instruction
#define PRINT_INST(out,instr,str,cycle)	\
  myfprintf(out, "%d: %s", cycle, str);		\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

#define PRINT_REG(out,reg,str,instr) \
  myfprintf(out, "reg#%d %s ", reg, str);	\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

/* VARIABLES */

//instruction queue for tomasulo
static instruction_t* instr_queue[INSTR_QUEUE_SIZE];

//number of instructions in the instruction queue
static int instr_queue_size = 0;

/* ECE552 Assignment 3 - BEGIN CODE */
//Going to make the IFQ a linked list. easier operations for pushing and popping
typedef struct Node{
    instruction_t* inst;
    struct Node* next;
}Node;

typedef struct{
    Node *head;
    Node *tail;
}instr_queue_list;

static instr_queue_list inst_queue;
void instr_push(instruction_t *newInst){
        //Check if we have an empty queue
        Node *newNode = (Node*)malloc(sizeof(Node)); 
        newNode->inst = newInst;
        newNode->next = NULL;
        instr_queue_size++;
        if(!inst_queue.head && !inst_queue.tail){
            inst_queue.head = newNode;
            inst_queue.tail = newNode;
            return;
        }
        if(inst_queue.tail){
            inst_queue.tail->next = newNode;
            inst_queue.tail = newNode;
        }
}

void instr_pop(){
    if(inst_queue.head){
        Node *toDelete = inst_queue.head;
        inst_queue.head = inst_queue.head->next;
        toDelete->next = NULL;
        free(toDelete);

        instr_queue_size--;
    }
}

instruction_t* instr_front(){
    if(inst_queue.head){
        return inst_queue.head->inst;
    }
}

bool instr_isEmpty(){
    return (!inst_queue.head && !inst_queue.tail);
}


/* ECE552 Assignment 3 - END CODE */

//reservation stations (each reservation station entry contains a pointer to an instruction)
static instruction_t* reservINT[RESERV_INT_SIZE];
static instruction_t* reservFP[RESERV_FP_SIZE];

//functional units
static instruction_t* fuINT[FU_INT_SIZE];
static instruction_t* fuFP[FU_FP_SIZE];

//common data bus
static instruction_t* commonDataBus = NULL;

//The map table keeps track of which instruction produces the value for each register
static instruction_t* map_table[MD_TOTAL_REGS];

//the index of the last instruction fetched
static int fetch_index = 0;

/* FUNCTIONAL UNITS */


/* RESERVATION STATIONS */


/* ECE552 Assignment 3 - BEGIN CODE */
void markRAWDependence(instruction_t *instr) {
    if(!instr){ //Passed in a NULL instruction
        return;
    }
    int i;
    //iterate through all input registers and see if there are entries in the map table
    //for the specific input register
    for(i = 0; i < 3; i++){
        instr->Q[i] = map_table[instr->r_in[i]];
    }
}
/* ECE552 Assignment 3 - END CODE */

/* 
 * Description: 
 * 	Checks if simulation is done by finishing the very last instruction
 *      Remember that simulation is done only if the entire pipeline is empty
 * Inputs:
 * 	sim_insn: the total number of instructions simulated
 * Returns:
 * 	True: if simulation is finished
 */
static bool is_simulation_done(counter_t sim_insn) {

  /* ECE552: YOUR CODE GOES HERE */

  return false; //ECE552: you can change this as needed; we've added this so the code provided to you compiles
}

bool isBusy(instruction_t *inst){
    assert(inst);
    return((inst->tom_cdb_cycle == 0)?true:false);
}

/* 
 * Description: 
 * 	Retires the instruction from writing to the Common Data Bus
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void CDB_To_retire(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */

}


/* 
 * Description: 
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void execute_To_CDB(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */

}

/* 
 * Description: 
 * 	Moves instruction(s) from the issue to the execute stage (if possible). We prioritize old instructions
 *      (in program order) over new ones, if they both contend for the same functional unit.
 *      All RAW dependences need to have been resolved with stalls before an instruction enters execute.
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void issue_To_execute(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
}

/* 
 * Description: 
 * 	Moves instruction(s) from the dispatch stage to the issue stage
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void dispatch_To_issue(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
    int i;
    for(i = 0; i < RESERV_INT_SIZE; i++){
        if(reservINT[i]){
           reservINT[i]->tom_issue_cycle = current_cycle;
        }
    }

    for(i = 0; i < RESERV_FP_SIZE; i++){
        if(reservFP[i]){
            reservFP[i]->tom_issue_cycle = current_cycle;
        }
    } 
}

/* 
 * Description: 
 * 	Grabs an instruction from the instruction trace (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	None
 */
void fetch(instruction_trace_t* trace) {

    /* ECE552: YOUR CODE GOES HERE */
    //I have valid index 
    instr_push(&(trace->table[fetch_index]));
    
    fetch_index++;
}

/* 
 * Description: 
 * 	Calls fetch and dispatches an instruction at the same cycle (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void fetch_To_dispatch(instruction_trace_t* trace, int current_cycle) {

    /* ECE552: YOUR CODE GOES HERE */
    if(instr_queue_size < 10){
        trace->table[fetch_index].tom_dispatch_cycle = current_cycle;
        fetch(trace);
    }
    
    //Resolve target and source
    
    if(USES_FP_FU(instr_front()->op)){
        int i;
        for(i = 0; i < RESERV_FP_SIZE; i++){
            if(!reservFP[i]){
                reservFP[i] = instr_front();
                markRAWDependence(reservFP[i]);
                instr_pop();
                break;
            }
        }
    }
    else if(USES_INT_FU(instr_front()->op)){
        int i;
        for(i = 0; i < RESERV_INT_SIZE; i++){
            if(!reservINT[i]){
                reservINT[i] = instr_front();
                markRAWDependence(reservINT[i]);
                instr_pop();
                break;
            }
        }
    }
    else if(IS_UNCOND_CTRL(instr_front()->op) ||
            IS_COND_CTRL(instr_front()->op)){
        instr_pop();
    }

}

/* 
 * Description: 
 * 	Performs a cycle-by-cycle simulation of the 4-stage pipeline
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	The total number of cycles it takes to execute the instructions.
 * Extra Notes:
 * 	sim_num_insn: the number of instructions in the trace
 */
counter_t runTomasulo(instruction_trace_t* trace)
{
    /* ECE552 Assignment 3 - BEGIN CODE */
    inst_queue.head = NULL;
    inst_queue.tail = NULL;
    /* ECE552 Assignment 3 - END CODE */
    
    //initialize instruction queue
    int i;
    for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
      instr_queue[i] = NULL;
    }

    //initialize reservation stations
    for (i = 0; i < RESERV_INT_SIZE; i++) {
        reservINT[i] = NULL;
    }

    for(i = 0; i < RESERV_FP_SIZE; i++) {
        reservFP[i] = NULL;
    }

    //initialize functional units
    for (i = 0; i < FU_INT_SIZE; i++) {
      fuINT[i] = NULL;
    }

    for (i = 0; i < FU_FP_SIZE; i++) {
      fuFP[i] = NULL;
    }

    //initialize map_table to no producers
    int reg;
    for (reg = 0; reg < MD_TOTAL_REGS; reg++) {
      map_table[reg] = NULL;
    }
    
    int cycle = 1;
    instruction_trace_t *currTrace = trace;
    while (true) {

       /* ECE552: YOUR CODE GOES HERE */

        /* ECE552 Assignment 3 - BEGIN CODE */
        if (is_simulation_done(sim_num_insn))
            break;
         
        dispatch_To_issue(cycle);
        if(fetch_index == currTrace->size){
            currTrace = currTrace->next;
            fetch_index = 0;
        }
        if(IS_TRAP(currTrace->table[fetch_index].op)){
            fetch_index++;
            continue;
        }
        fetch_To_dispatch(currTrace,cycle);
        cycle++;

        /* ECE552 Assignment 3 - END CODE */
    }
    
    return cycle;
}
