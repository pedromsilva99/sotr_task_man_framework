/* 
 * Pedro Silva 89228
 * Pedro Gonçalves 88859
 *
 * SOTR 2021/22
 * 
 * Task Management framework for FreeRTOS
 * 
 * FREERTOS demo for ChipKit MAX32 board
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
#define HIGHER_PRIORITY         ( tskIDLE_PRIORITY + 4 ) //com 5 nao funciona
#define MEDIUM_HIGH_PRIORITY	( tskIDLE_PRIORITY + 3 )
#define MEDIUM_PRIORITY         ( tskIDLE_PRIORITY + 2 )
#define MEDIUM_LOW_PRIORITY     ( tskIDLE_PRIORITY + 1 )
#define LOWER_PRIORITY          ( tskIDLE_PRIORITY )

#define SYSCLK  80000000L // System clock frequency, in Hz
#define PBCLOCK 40000000L // Peripheral Bus Clock frequency, in Hz
#define MAX_TASKS 6

void TMAN_TaskWaitPeriod(int tar);
void TMAN_CloseInternal(void);
void TMAN_TaskStatsInternal(int task_id);
void consuming_task(void *pvParam);

// Variable declarations;
int tick;
int tman_tick=0, started_tasks=0;
int close_tick, stats_tick;

typedef struct{
    int period;
    int deadline;
    int phase;
    int prec_executed; // 0 -> no, 1 -> yes
    int executed_once;
    char *name;
    int id;
    int activations;
    int deadlines_missed;
    int state; //0->blocked, 1->suspended, 2->ready
    int prec_signal;
    char *prec;
    
}TASK;

TASK t1 [MAX_TASKS];

typedef struct{
    int taskId, maxTasks;
    TASK tasks [MAX_TASKS];
}TMAN;

TaskHandle_t xHandle [MAX_TASKS];
QueueHandle_t xQueue1;

/* Tasks*/

void scheduler(void *pvParam) {
    TickType_t time = xTaskGetTickCount();
    xTaskCreate( consuming_task, ( const signed char * const ) t1[0].name, configMINIMAL_STACK_SIZE, NULL, HIGHER_PRIORITY, &xHandle[0] );
    xTaskCreate( consuming_task, ( const signed char * const ) t1[1].name, configMINIMAL_STACK_SIZE, NULL, HIGHER_PRIORITY, &xHandle[1] );
    xTaskCreate( consuming_task, ( const signed char * const ) t1[2].name, configMINIMAL_STACK_SIZE, NULL, MEDIUM_HIGH_PRIORITY, &xHandle[2] );
    xTaskCreate( consuming_task, ( const signed char * const ) t1[3].name, configMINIMAL_STACK_SIZE, NULL, MEDIUM_HIGH_PRIORITY, &xHandle[3] );
    xTaskCreate( consuming_task, ( const signed char * const ) t1[4].name, configMINIMAL_STACK_SIZE, NULL, MEDIUM_LOW_PRIORITY, &xHandle[4] );
    xTaskCreate( consuming_task, ( const signed char * const ) t1[5].name, configMINIMAL_STACK_SIZE, NULL, LOWER_PRIORITY, &xHandle[5] );
    
    for(int i=0; i<started_tasks; i++){
        vTaskSuspend(xHandle[i]);
        t1[i].state=0;
    }
    for(;;) {
        tman_tick++;
        //printf("%d ticks\n", tman_tick);
        
        for(int i=0; i<started_tasks; i++){
            /* Ver quais são as tasks que devem correr neste ciclo*/
            if((tman_tick%t1[i].period)==t1[i].phase && t1[i].prec_signal==0){
                vTaskResume(xHandle[i] );
                t1[i].state=2;
            }
            if ((tman_tick%t1[i].period) == t1[i].phase) {
                t1[i].prec_executed = 0;
            }
            if (tman_tick == close_tick)
                TMAN_CloseInternal();
            if (tman_tick % stats_tick == 0)
                TMAN_TaskStatsInternal(i);
        }
        
        for(int j=0; j<started_tasks; j++){
            for(int i=0; i<started_tasks; i++){
                /* Ver quais são as tasks que têm que ficar suspensas porque têm precedências*/
                if((tman_tick%t1[i].period)==t1[i].phase && t1[i].prec_signal==1){
                    for(int k=0; k<started_tasks;k++){
                        if(strcmp(t1[i].prec, t1[k].name) == 0){
                            
                            // Se a task que precede esta a correr ou suspensa, esta tambem fica suspensa
                            if(t1[k].state==2 || t1[k].state==1){
                                t1[i].state=1;
                                // printf("%s espera porque tem precedencias %s\n", t1[i].name, t1[k].name);
                            } else if(t1[k].prec_executed == 1) {
                                t1[i].state = 2;
                                vTaskResume(xHandle[i] );
                            } else if (t1[k].prec_executed == 0) {
                                t1[i].state = 1;
                            }
                        }
                    }
                }
                /* Ver quais são as tasks que estão suspensas e já podem retornar a sua atividade*/
                else if((t1[i].state==1)){
                    for(int k=0; k<started_tasks;k++){
                        if(strcmp(t1[i].prec, t1[k].name) == 0){
                            if(t1[k].prec_executed == 1){
                                t1[i].state=2;
                                // printf("%s ja nao precisa  %s\n", t1[i].name, t1[k].name);
                                vTaskResume(xHandle[i] );
                            }
                        }
                    }
                }
            }
            if (tman_tick >= (t1[j].deadline + t1[j].phase) && t1[j].executed_once == 0 && tman_tick % t1[j].period == t1[j].phase) {
                printf("Deadline missed for Task %s\n", t1[j].name);
                t1[j].deadlines_missed++;
            }
        }
        
        vTaskDelayUntil(&time, tick);   
    }
}

void consuming_task(void *pvParam) {
    
    for(;;){
        //printf("e");
        for(int i=0; i<started_tasks; i++){
            if(t1[i].state==2){
                int sum = 1;
                for(int j=1;j<100;j++){
                    for(int k=1;k<18;k++){
                        sum = sum * j * k;
                    }
                }
                char buffer[50];
                sprintf(buffer, "%s, %d\n", t1[i].name, tman_tick);
                
                if( xQueue1 != 0 ){
                    if( xQueueSend( xQueue1,( void * ) &buffer,( TickType_t ) 0 ) != pdPASS ){
                        /* Failed to post the message, even after 10 ticks. */
                        // printf("Failed to load message to queue\n");
                    }
                    
                }
                TMAN_TaskWaitPeriod(i);
            }   
        }
    }
}

void printing_queue(void *pvParam) {
    char xStructure[50];
    for(;;){
        
        if( xQueueReceive( xQueue1,xStructure,( TickType_t ) 10 ) == pdPASS ){
         /* xStructure now contains a copy of xMessage. */
            printf("%s", xStructure);    
        }
        
        
    }
}

TMAN TMAN_Init(int ticks_tman){
    
    TMAN tm;
    tick = ticks_tman;
    tm.maxTasks=MAX_TASKS;
    tm.taskId=0;
    xTaskCreate( scheduler, ( const signed char * const ) "ticking", configMINIMAL_STACK_SIZE, NULL, HIGHER_PRIORITY, NULL );
    xTaskCreate( printing_queue, ( const signed char * const ) "printing", configMINIMAL_STACK_SIZE, NULL, LOWER_PRIORITY, NULL );
    return tm;
}

void TMAN_Close(int tick_c){
    close_tick = tick_c;
}

void TMAN_CloseInternal(void){
    for (int i = 0; i < started_tasks; i++) {
        if( xHandle[i] != NULL )
            vTaskDelete( xHandle[i] );
    }
    started_tasks = 0;
    printf("TMAN Closed!\n");
    exit(0);
}

TMAN TMAN_TaskAdd(TMAN t, char *name){
    TASK tas;
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

void TMAN_TaskStats(int tick_s) {
    stats_tick = tick_s;
}

void TMAN_TaskStatsInternal(int task_id) {
    printf("Task %s total activations: %d, deadlines missed: %d\n", t1[task_id].name, t1[task_id].activations, t1[task_id].deadlines_missed);
}

void TMAN_TaskWaitPeriod(int tar) {
    t1[tar].state = 0;
    t1[tar].prec_executed = 1;
    t1[tar].activations++;
    if (t1[tar].executed_once == 0)
        t1[tar].executed_once = 1;
    if( xHandle[tar] != NULL ){                
        vTaskSuspend( xHandle[tar] );
    }
}

// phase is the offset, add state (blocked ...)
TMAN TMAN_TaskRegisterAttributes(TMAN t, int period, int phase, int deadline, char *namec, char *prec_task){
    assert(period > phase);
//    assert(deadline > phase);
    if (period > deadline) {
        printf("System is not feasible!\n");
        exit(0);
    }
    for(int i = 0; i<t.maxTasks;i++){
        if (strcmp(namec, t.tasks[i].name) == 0){
            t.tasks[i].period=period;
            t.tasks[i].deadline=deadline;
            t.tasks[i].phase=phase;
            t.tasks[i].prec_executed = 0;
            t.tasks[i].executed_once = 0;
            t.tasks[i].activations = 0;
            t.tasks[i].deadlines_missed = 0;
            if (strcmp(prec_task, "") != 0){
                t.tasks[i].prec_signal = 1;
                t.tasks[i].prec = prec_task;
                for(int j = 0; j<t.maxTasks;j++){
                    if (strcmp(prec_task, t.tasks[j].name) == 0){
                        if(t.tasks[i].period<t.tasks[j].period)
                            t.tasks[i].period=t.tasks[j].period;
                            t.tasks[i].phase=t.tasks[j].phase;
                    }
                }
            }
            break;
        }
    }
    
    return t;
}

TMAN TMAN_TaskStart(TMAN t, char *name){
    started_tasks++;
    for(int i = 0; i<t.maxTasks;i++){
        if (strcmp(name, t.tasks[i].name) == 0){
            t1[i] = t.tasks[i];
            
            break;
        }
    }
    
    
}

void uart(void){

    // Init UART and redirect tdin/stdot/stderr to UART
    if(UartInit(PBCLOCK, 115200) != UART_SUCCESS) {
        PORTAbits.RA3 = 1; // If Led active error initializing UART
        while(1);
    }
    __XC_UART = 1; /* Redirect stdin/stdout/stderr to UART1*/
}

int mainTman( void )
{
    uart(); //print to the terminal      
    xQueue1 = xQueueCreate(10,50);
    for(int i=0; i<MAX_TASKS ;i++){
        xHandle[i]=NULL;
    }
    TMAN tm = TMAN_Init(100);
    TASK task1 = {80,90,12};
    printf("\n\nStarting TMAN simulation\n");
    tm=TMAN_TaskAdd(tm, "A");
    tm=TMAN_TaskAdd(tm, "B");
    tm=TMAN_TaskAdd(tm, "C");
    tm=TMAN_TaskAdd(tm, "D");
    tm=TMAN_TaskAdd(tm, "E");
    tm=TMAN_TaskAdd(tm, "F");
    printf("Task added\n");
    tm=TMAN_TaskRegisterAttributes(tm, 1, 0, 400,"A","");
    tm=TMAN_TaskRegisterAttributes(tm, 1, 0, 400,"B","");
    tm=TMAN_TaskRegisterAttributes(tm, 2, 1, 400,"D","");
    tm=TMAN_TaskRegisterAttributes(tm, 5, 2, 400,"E","");
    tm=TMAN_TaskRegisterAttributes(tm, 2, 0, 400,"C","E");
    tm=TMAN_TaskRegisterAttributes(tm, 10, 0, 400,"F","");
    printf("Attr added\n");
    printf("%s: Period: %d, Phase: %d\n", tm.tasks[0].name, tm.tasks[0].period, tm.tasks[0].phase);
    printf("%s: Period: %d, Phase: %d\n", tm.tasks[1].name, tm.tasks[1].period, tm.tasks[1].phase);
    printf("Initiate Task\n");
    tm=TMAN_TaskStart(tm,"A");
    tm=TMAN_TaskStart(tm,"B");
    tm=TMAN_TaskStart(tm,"C");
    tm=TMAN_TaskStart(tm,"D");
    tm=TMAN_TaskStart(tm,"E");
    tm=TMAN_TaskStart(tm,"F");
    
    TMAN_Close(55);
    TMAN_TaskStats(25);
    /* Finally start the scheduler. */
    vTaskStartScheduler();
    
    printf("End Task\n");
    
	return 0;
}