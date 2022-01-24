#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 5

typedef struct{
    int period;
    int deadline;
    int phase;
    char *name;
    int id;
}TASK;

typedef struct{
    int nTasksCreated, nTasksDeleted, nTasksActive, nTicks, taskId, maxTasks;
    TASK tasks [MAX_TASKS];
}TMAN;

TMAN TMAN_Init(int ticks_tman){
    TMAN tm;
    // tm.tasks = calloc(maxTasks, sizeof(int)); //https://stackoverflow.com/questions/35801119/c-struct-with-array-size-determined-at-compile-time
    tm.nTasksCreated=0;
    tm.nTasksDeleted=0;
    tm.nTasksActive=0;
    tm.nTicks=0;
    tm.maxTasks=MAX_TASKS;
    tm.taskId=0;

    return tm;
}

TMAN TMAN_Close(TMAN t){
    // free(t.tasks);
    t.nTasksCreated=0;
    t.nTasksDeleted=0;
    t.nTasksActive=0;
    t.nTicks=0;
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

TMAN TMAN_TaskRegisterAttributes(TMAN t, int period, int phase, int deadline, char *namec){//}, uint8_t *precedences){

    for(int i = 0; i<t.maxTasks;i++){
        if (strcmp(namec, t.tasks[i].name)==0){
            t.tasks[i].period=period;
            t.tasks[i].deadline=deadline;
            t.tasks[i].phase=phase;
            break;
        }
    }
    return t;
}

int main(int argc, char *argv[])
{
    TMAN tm = TMAN_Init(0);
    TASK task1 = {80,90,12};
    char *name = "Task_1";
    char *name2 = "Task_2";

    tm=TMAN_TaskAdd(tm, name);
    tm=TMAN_TaskAdd(tm, name2);

    printf("Task added\n");
    tm=TMAN_TaskRegisterAttributes(tm,30,40,50,"Task_1");
    printf("Attr added\n");

    printf("%d,%d\n", task1.period, task1.deadline);
    printf("%d,%d,%s,%d,%d\n", tm.maxTasks, tm.nTasksCreated, tm.tasks[0].name, tm.tasks[0].period, tm.tasks[0].phase);
    tm=TMAN_TaskDelete(tm,name2);
    printf("%d,%d,%s,%d,%d\n", tm.maxTasks, tm.nTasksCreated, tm.tasks[1].name, tm.tasks[1].period, tm.tasks[1].phase);
}
