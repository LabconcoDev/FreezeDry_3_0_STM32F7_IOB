/*
 * Copyright (c) 2026 Labconco Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/logging/log.h>
#include "task3_handler.h"
#include "stack_debug.h"
LOG_MODULE_REGISTER(task3, LOG_LEVEL_INF);


struct counter_object {
   struct smf_ctx ctx;
   /* All User Defined Data Follows */
   int counter;
};

/*------------------------------------------------------------------
 * State IDs
 *----------------------------------------------------------------*/
enum counter_states {
    COUNT_UP,
    COUNT_DOWN,
};

static struct counter_object counter_obj;

static const struct smf_state states[];

/*-----------------------------
 * COUNT_UP State
 *----------------------------*/
static void count_up_entry(void *obj)
{
    printk("\nEntering COUNT_UP\n");
}

static enum smf_state_result count_up_run(void *obj)
{
    struct counter_object *s = obj;

    //printk("COUNT_UP: %d\n", s->counter);

    if (s->counter >= 5) {
        smf_set_state(SMF_CTX(s), &states[COUNT_DOWN]);
        return SMF_EVENT_HANDLED;
    }

    s->counter++;
    return SMF_EVENT_PROPAGATE;
}

/*-----------------------------
 * COUNT_DOWN State
 *----------------------------*/
static void count_down_entry(void *obj)
{
    //printk("\nEntering COUNT_DOWN\n");
}

static enum smf_state_result count_down_run(void *obj)
{
    struct counter_object *s = obj;

    //printk("COUNT_DOWN: %d\n", s->counter);

    if (s->counter <= 0) {
        smf_set_state(SMF_CTX(s), &states[COUNT_UP]);
        return SMF_EVENT_HANDLED;
    }

    s->counter--;
    return SMF_EVENT_PROPAGATE;
}

/*-----------------------------
 * State Table
 *----------------------------*/
static const struct smf_state states[] = {
    [COUNT_UP] = SMF_CREATE_STATE(
        count_up_entry,
        count_up_run,
        NULL,
        NULL,
        NULL),

    [COUNT_DOWN] = SMF_CREATE_STATE(
        count_down_entry,
        count_down_run,
        NULL,
        NULL,
        NULL),
};

void counter_smf_init(void)
{
    counter_obj.counter = 0;

    smf_set_initial(
        SMF_CTX(&counter_obj),
        &states[COUNT_UP]);
}

void counter_smf_run(void)
{
    smf_run_state(SMF_CTX(&counter_obj));
}

void task3_smf(void)
{
    counter_smf_run();
}

int task3_init(void)
{
    print_stack_usage();
    counter_smf_init();
    return 0;
}