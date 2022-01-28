
/*
 * Paulo Pedreiras, Sept/2021
 * 
 * Pedro Silva 89228
 * Pedro Gonçalves 88859
 *
 * FREERTOS demo for ChipKit MAX32 board
 * - Creates two periodic tasks
 * - One toggles Led LD4, other is a long (interfering)task that 
 *      activates LD5 when executing 
 * - When the interfering task has higher priority interference becomes visible
 *      - LD4 does not blink at the right rate
 *
 * Environment:
 * - MPLAB X IDE v5.45
 * - XC32 V2.50
 * - FreeRTOS V202107.00
 *
 *
 */

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <xc.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* App includes */
#include "../UART/uart.h"

/* Priorities of the demo application tasks (high numb. -> high prio.) */
#define ACQ_PRIORITY	( tskIDLE_PRIORITY + 3 )
#define PROC_PRIORITY	( tskIDLE_PRIORITY + 2 )
#define OUT_PRIORITY	( tskIDLE_PRIORITY + 1 )

#define SYSCLK  80000000L // System clock frequency, in Hz
#define PBCLOCK 40000000L // Peripheral Bus Clock frequency, in Hz
#define MAX_TASKS 5


// Variable declarations;
SemaphoreHandle_t sem1;
SemaphoreHandle_t sem2;
float res; // Sampled volatge
float proc_out; //Value that is processed in proc task
int tick;
int tman_tick=0, started_tasks=0;
char *tname;

void TMAN_TaskWaitPeriod(int tar);
void consuming_task(void *pvParam);

typedef struct{
    int period;
    int deadline;
    int phase;
    char *name;
    int id;
    int state; //0->blocked, 1->suspended, 2->ready, 3->running
    int prec_signal;
    char *prec;
    
}TASK;

TASK t1 [MAX_TASKS];

typedef struct{
    int nTasksCreated, nTasksDeleted, nTasksActive, tick_period, taskId, maxTasks;
    TASK tasks [MAX_TASKS];
}TMAN;

TaskHandle_t xHandle [MAX_TASKS];
QueueHandle_t xQueue1;
//xMessage.ucMessageID = 0xab;
//memset( &( xMessage.ucData ), 0x12, 20 );
//QueueHandle_t xQueue1;
//
void simulate(void *pvParam) {
    TickType_t time = xTaskGetTickCount();
    for(int i=0; i<started_tasks; i++){
        xTaskCreate( consuming_task, ( const signed char * const ) t1[i].name, configMINIMAL_STACK_SIZE, NULL, PROC_PRIORITY, &xHandle[i] );
        t1[i].state=0;
    }
    for(;;) {
        tman_tick++;
        //printf("%d ticks\n", tman_tick);
        /* Ver quais são as tasks que devem correr neste ciclo*/
        for(int i=0; i<started_tasks; i++){
            if((tman_tick%t1[i].period)==t1[i].phase){
                
                vTaskResume(xHandle[i] );
                t1[i].state=2;
            }
        }
        
        vTaskDelayUntil(&time, tick);   
    }
}

void consuming_task(void *pvParam) {
    
    for(;;){
        for(int i=0; i<started_tasks; i++){
            if(t1[i].state==2){
                int sum = 1;
                //for(int j=1;j<1000;j++){
                //    for(int k=1;k<1000;k++){
                //        sum=sum*j*k;
                //    }
                //}
                char buffer[50];
                sprintf(buffer, "%s, %d\n", t1[i].name, tman_tick);
                
                if( xQueue1 != 0 ){
                    if( xQueueSend( xQueue1,( void * ) &buffer,( TickType_t ) 0 ) != pdPASS ){
                        /* Failed to post the message, even after 10 ticks. */
                    }
                    printf("%s", buffer);
                }
                TMAN_TaskWaitPeriod(i);
            }   
        }
    }
}

void printing_queue(void *pvParam) {
    void * xRxedStructure;
    char *ptemp;
    for(;;){
        
        if( xQueueReceive( xQueue1,& (xRxedStructure),( TickType_t ) 10 ) == pdPASS ){
         /* xRxedStructure now contains a copy of xMessage. */
            //printf("Da 3\n");
            ptemp = (char *)xRxedStructure;
            //printf("Value:  %d\n", *xRxedStructure );
            //printf("OH FUCK %s\n", ptemp);    
        }
        
        
    }
}

TMAN TMAN_Init(int ticks_tman){
    
    TMAN tm;
    tm.nTasksCreated=0;
    tm.nTasksDeleted=0;
    tm.nTasksActive=0;
    tick = ticks_tman;
    tm.maxTasks=MAX_TASKS;
    tm.taskId=0;
    xTaskCreate( simulate, ( const signed char * const ) "ticking", configMINIMAL_STACK_SIZE, NULL, ACQ_PRIORITY, NULL );
    xTaskCreate( printing_queue, ( const signed char * const ) "printing", configMINIMAL_STACK_SIZE, NULL, OUT_PRIORITY, NULL );
    return tm;
}

TMAN TMAN_Close(TMAN t){
    // free(t.tasks);
    t.nTasksCreated=0;
    t.nTasksDeleted=0;
    t.nTasksActive=0;
    t.tick_period=0;
    t.taskId=0;
    return t;
}

TMAN TMAN_TaskAdd(TMAN t, char *name){
    TASK tas;
    t.nTasksCreated++;
    tas.name = name;
    tas.period=-1;
    tas.deadline=-1;
    tas.phase=-1;
    tas.id = t.taskId;
    tas.state = 0;
    tas.prec_signal = 0;
    tas.prec = "";
    t.tasks[t.taskId] = tas;
    t.taskId++;
    return t;
}

TMAN TMAN_TaskDelete(TMAN t, char *namec){
    for(int i = 0; i<t.maxTasks;i++){
        if (strcmp(namec, t.tasks[i].name)==0){
            t.tasks[i].period=-1;
            t.tasks[i].deadline=-1;
            t.tasks[i].phase=-1;
            t.tasks[i].name="(null)";
            break;
        }
    }
    t.nTasksDeleted++;
    return t;
}

void TMAN_TaskWaitPeriod(int tar) {
    t1[tar].state = 0;
    if( xHandle[tar] != NULL ){                
        vTaskSuspend( xHandle[tar] );
    }
}

// phase is the offset, add state (blocked ...)
TMAN TMAN_TaskRegisterAttributes(TMAN t, int period, int phase, int deadline, char *namec){//), char *prec_task){
    assert(period > phase);
    assert(deadline > phase);
    for(int i = 0; i<t.maxTasks;i++){
        if (strcmp(namec, t.tasks[i].name) == 0){
            t.tasks[i].period=period;
            t.tasks[i].deadline=deadline;
            t.tasks[i].phase=phase;
            //if (strcmp(prec_task, "") != 0){
                //t.tasks[i].prec_signal = 1;
                //t.tasks[i].prec = prec_task;
            //}
            break;
        }
    }
    
    return t;
}

TMAN TMAN_TaskStart(TMAN t, char *name){
    started_tasks++;
    for(int i = 0; i<t.maxTasks;i++){
        if (strcmp(name, t.tasks[i].name) == 0){
            t.tasks[i].state=3;
            t1[i] = t.tasks[i];
            
            break;
        }
    }
    
    
}

/* Tasks*/

void uart(void){

    // Init UART and redirect tdin/stdot/stderr to UART
    if(UartInit(PBCLOCK, 115200) != UART_SUCCESS) {
        PORTAbits.RA3 = 1; // If Led active error initializing UART
        while(1);
    }
    __XC_UART = 1; /* Redirect stdin/stdout/stderr to UART1*/
}

int mainSetrLedBlink( void )
{
    uart(); //print to the terminal      
    xQueue1 = xQueueCreate(10,50);
    for(int i=0; i<MAX_TASKS ;i++){
        xHandle[i]=NULL;
    }
    TMAN tm = TMAN_Init(1000);
    TASK task1 = {80,90,12};
    printf("\n\nStarting simulation\n");
    tm=TMAN_TaskAdd(tm, "A");
    tm=TMAN_TaskAdd(tm, "B");
    tm=TMAN_TaskAdd(tm, "C");
    printf("Task added\n");
    tm=TMAN_TaskRegisterAttributes(tm, 4, 2, 4,"A");
    tm=TMAN_TaskRegisterAttributes(tm, 4, 2, 4,"B");//,"A");
    tm=TMAN_TaskRegisterAttributes(tm, 4, 2, 4,"C");//,"");
    printf("Attr added\n");
    printf("%s: Period: %d, Phase: %d\n", tm.tasks[0].name, tm.tasks[0].period, tm.tasks[0].phase);
    printf("%s: Period: %d, Phase: %d\n", tm.tasks[1].name, tm.tasks[1].period, tm.tasks[1].phase);
    printf("Initiate Task\n");
    tm=TMAN_TaskStart(tm,"A");
    tm=TMAN_TaskStart(tm,"B");
    tm=TMAN_TaskStart(tm,"C");
    
    /* Finally start the scheduler. */
    vTaskStartScheduler();
    
    //TMAN_TaskWaitPeriod(tm, tm.tasks[0].period);
    printf("End Task\n");
    
	return 0;
}