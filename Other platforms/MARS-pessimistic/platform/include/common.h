/* Copyright (c) 2012. MRSG Team. All rights reserved. */

/* This file is part of MRSG.

MRSG is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MRSG is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MRSG.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef MRSG_COMMON_H
#define MRSG_COMMON_H

#include <msg/msg.h>
#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/asserts.h>
#include "mrsg.h"

// #define VERBOSE

/* Hearbeat parameters. */
#define HEARTBEAT_MIN_INTERVAL 3
#define HEARTBEAT_TIMEOUT 600

/* Short message names. */
#define SMS_GET_CHUNK "SMS-GC"
#define SMS_GET_INTER_PAIRS "SMS-GIP"
#define SMS_HEARTBEAT "SMS-HB"
#define SMS_TASK "SMS-T"
#define SMS_TASK_DONE_CORRECT "SMS-TD-OK"
#define SMS_TASK_DONE_BYZANTINE "SMS-TD-BYZ"
#define SMS_FINISH "SMS-F"
#define SMS_DLT_BLOCK "SMS-DLT_BLOCK"

#define NONE (-1)
#define MAX_SPECULATIVE_COPIES 3
#define TRIGGER_TIMEOUT_SPECULATIVE_MAP 60
#define TRIGGER_TIMEOUT_SPECULATIVE_REDUCE 10

/* Mailbox related. */
#define MAILBOX_ALIAS_SIZE 256
#define MASTER_MAILBOX "MASTER"
#define DLT_MAILBOX "DLT"
#define DATANODE_MAILBOX "%zu:DN"
#define MAP_TASKTRACKER_MAILBOX "%zu:TTMAP"
#define REDUCE_TASKTRACKER_MAILBOX "%zu:TTREDUCE"
#define COMPLETED_TASKTRACKER_MAILBOX "%zu:TTCOMPLETED"
#define TASK_MAILBOX "%zu:%d"

/** @brief  Possible task status. */
enum task_status_e {
    /* The initial status must be the first enum. */
    T_STATUS_PENDING, // waiting
    T_STATUS_TIP,     // asked execution
    T_STATUS_TIP_SLOW,// slow, to be scheduled for speculative tasks
    T_STATUS_DONE     // done
};

enum task_type_e {
    LOCAL,
    REMOTE,
    LOCAL_SPEC,
    REMOTE_SPEC,
    NORMAL,
    SPECULATIVE,
    NO_TASK
};

/** @brief  Information sent by the workers with every heartbeat.
 *  Slots available?
 *  */
struct heartbeat_s {
    int  slots_av[2];
};

typedef struct heartbeat_s* heartbeat_t;

struct config_s {
    double         chunk_size;
    double         grid_average_speed;
    double         grid_cpu_power;
    int            chunk_count;
    int            chunk_replicas;
    int            heartbeat_interval;
    int            amount_of_tasks[2];
    int            number_of_workers;
    int            slots[2];
    int            initialized;
    int            byzantine;    // number of byzantine nodes that the system need to tollerate
    int 		   real_byzantine; // real number of byznatine nodes in the system
    int 		   block_size;   // number of messages per block
    int 		   block_period; // number of seconds between a block and the other
    msg_host_t*    workers;
    int 		   random_seed;
} config;

struct job_s {
    int           finished;
    int 		  worker_active_count;
    int           tasks_pending[2];
    int*          task_instances[2]; //[phase][task_id] -> running tasks
    int*          task_replicas_instances[2]; //[phase][task_id] -> running replicas for tasks
    int*          task_confirmations[2]; //[phase][task_id] -> tasks that completed this task with the same result
    int* 		  task_byzantine_confirmations[2]; //[phase][task_id] -> confirmations by byzantine attackers
    int*          task_status[2]; //[phase][task_id] -> pending, running, timeout, completed
    msg_task_t**  task_list[2];//[phase][task_id][worker id] -> collect the task descriptions for each task assigned to some node
    size_t**      map_output;//[map_id][reduce_id] -> collect which map has produced which keys and how much data it has produced for that key
    heartbeat_t   heartbeats;
    int* 		  byzantine_flag; //flag the byzantine nodes, so to mark which one has to misbehave
} job;

/** @brief  Information sent as the task data. */
struct task_info_s {
    enum phase_e  phase;
    size_t        id;
    size_t        src;
    size_t        wid;
    size_t		  map_id; //used only on the request to the namenode for an intermediate map result... mom, forgive me for the bad solution!
    int           pid;
    msg_task_t    task;
    size_t*       map_output_copied;
    double        shuffle_end;
};

typedef struct task_info_s* task_info_t;

struct DLT_block_s {
	msg_host_t* 	original_senders; // original transaction sender, as this structure is thought to be relayed by the DLT or someone else
    msg_task_t* 	original_messages;// original message - this structure is just an envelope
    int 			size;
};

typedef struct DLT_block_s* DLT_block_t;

struct stats_s {
    int   map_local;
    int   map_remote;
    int   map_spec_l;
    int   map_spec_r;
    int   reduce_normal;
    int   reduce_spec;
    int   reduce_local_map_result;
    int   reduce_remote_map_result;
} stats;

struct user_s {
    double (*task_cost_f)(enum phase_e phase, size_t tid, size_t wid);
    void (*dfs_f)(char** dfs_matrix, size_t chunks, size_t workers, int replicas);
    int (*map_output_f)(size_t mid, size_t rid);
    size_t (*scheduler_f)(enum phase_e phase, size_t wid);
} user;


/** 
 * @brief  Send a message/task.
 * @param  str      The message.
 * @param  cpu      The amount of cpu required by the task.
 * @param  net      The message size in bytes.
 * @param  data     Any data to attatch to the message.
 * @param  mailbox  The destination mailbox alias.
 * @return The MSG status of the operation.
 */
msg_error_t send (const char* str, double cpu, double net, void* data, const char* mailbox);

/** 
 * @brief  Send a short message, of size zero.
 * @param  str      The message.
 * @param  mailbox  The destination mailbox alias.
 * @return The MSG status of the operation.
 */
msg_error_t send_sms (const char* str, const char* mailbox);

/** 
 * @brief  Receive a message/task from a mailbox.
 * @param  msg      Where to store the received message.
 * @param  mailbox  The mailbox alias.
 * @return The status of the transfer.
 */
msg_error_t receive (msg_task_t* msg, const char* mailbox);

msg_error_t receiveTimeout(msg_task_t* msg, const char* mailbox, double timeout);

/** 
 * @brief  Compare the message from a task with a string.
 * @param  msg  The message/task.
 * @param  str  The string to compare with.
 * @return A positive value if matches, zero if doesn't.
 */
int message_is (msg_task_t msg, const char* str);

/**
 * @brief  Return the maximum of two values.
 */
int maxval (int a, int b);

size_t map_output_size (size_t mid);

size_t reduce_input_size (size_t rid);

enum task_type_e get_task_type (enum phase_e phase, size_t tid, size_t wid);

int number_of_task_replicas();

int *capacity[2]; //[PHASE][WORKER] number of free slots in that worker

typedef struct w_queues_worker_s {
	xbt_queue_t map_tasks_queue;
	xbt_queue_t reduce_tasks_queue;
	int size_queue_map;
	int size_queue_reduce;
} w_queue_worker_t;

w_queue_worker_t*  w_queue_workers; // [worker]

#endif /* !MRSG_COMMON_H */

// vim: set ts=8 sw=4:
