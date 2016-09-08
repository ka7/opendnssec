/*
 * Copyright (c) 2009 NLNet Labs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * Tasks.
 *
 */

#include "config.h"
#include "scheduler/task.h"
#include "scheduler/schedule.h"
#include "status.h"
#include "duration.h"
#include "file.h"
#include "log.h"
#include "signer/zone.h"

static const char* task_str = "task";
static pthread_mutex_t worklock = PTHREAD_MUTEX_INITIALIZER;

const char* TASK_CLASS_SIGNER = "signer";
const char* TASK_NONE = "[ignore]";
const char* TASK_SIGNCONF = "[configure]";
const char* TASK_READ = "[read]";
const char* TASK_NSECIFY = "[???]";
const char* TASK_SIGN = "[sign]";
const char* TASK_WRITE = "[write]";

task_type*
task_create(const char *owner, char const *class, char const *type,
    time_t (*callback)(char const *owner, void *userdata, void *context),
    void *userdata, void (*freedata)(void *userdata), time_t due_date)
{
    task_type* task;
    ods_log_assert(owner);
    ods_log_assert(class);
    ods_log_assert(type);

    CHECKALLOC(task = (task_type*) malloc(sizeof(task_type)));;
    task->owner = owner; /* TODO: each call to task_create needs to strdup this, but the free is inside task_destroy */
    task->class = class;
    task->type = type;
    task->callback = callback;
    task->userdata = userdata;
    task->freedata = freedata;
    task->due_date = due_date;
    task->lock = NULL;

    task->backoff = 0;
    task->flush = 0;

    return task;
}

void
task_destroy(task_type* task)
{
    ods_log_assert(task);
    free((void*)task->owner);
    if (task->freedata)
        task->freedata((void*)task->userdata);
    free(task);
}

time_t
task_execute(task_type *task, void *context)
{
    time_t t;
    ods_log_assert(task);
    /* We'll allow a task without callback, just don't reschedule. */
    if (!task->callback) {
        return -1;
    }
    ods_log_assert(task->owner);

    /*
     * It is sad but we need worklock to prevent concurrent database
     * access. Our code is not able to handle that properly. (we can't
     * really tell the difference between an error and nodata.) Once we
     * fixed our database backend this lock can be removed.
     * */
    if(task->lock) {
        pthread_mutex_lock(task->lock);
        t = task->callback(task->owner, task->userdata, context);
        pthread_mutex_unlock(task->lock);
    } else {
        t = task->callback(task->owner, task->userdata, context);
    }

    return t;
}

int
task_compare(const void* a, const void* b)
{
    task_type* x = (task_type*)a;
    task_type* y = (task_type*)b;

    ods_log_assert(x);
    ods_log_assert(y);

    /* order task on time, what to do, dname */
    if (x->due_date != y->due_date) {
        return (int) x->due_date - y->due_date;
    }
    if (strcmp(x->type, y->type)) {
        return strcmp(x->type, y->type);
    }
    /* this is unfair, it prioritizes zones that are first in canonical line */
    return strcmp(x->owner, y->owner);
}

void
task_log(task_type* task)
{
    char* strtime = NULL;

    if (task) {
        strtime = ctime(&task->due_date);
        if (strtime) {
            strtime[strlen(strtime)-1] = '\0';
        }
        ods_log_debug("[%s] %s %s I will %s zone %s", task_str,
            task->flush?"Flush":"On", strtime?strtime:"(null)",
            task->type, task->owner);
    }
}
