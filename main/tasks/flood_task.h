/**
 * @file flood_task.h
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file uses the pump driver and EC driver to flood the pot but not run the pump dry.
 * @version 0.1
 * @date 2025-04-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */


#ifndef FLOOD_TASK_H
#define FLOOD_TASK_H

typedef enum { 
    INIT,
    AWAIT_FLOOD_SIGNAL,
    FLOODING,
    NUM_STATES,
} flood_state_t;


void init_flood_task();


flood_state_t flood_status();

void begin_flooding();

void stop_flooding();

#endif // FLOOD_TASK_H