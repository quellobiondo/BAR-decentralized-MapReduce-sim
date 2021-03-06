#ifndef MRSG_COMMON_H
#define MRSG_COMMON_H

#include <msg/msg.h>
#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/asserts.h>
#include "mrsg.h"

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

#define NONE (-1)
#define MAX_SPECULATIVE_COPIES 3
#define TRIGGER_TIMEOUT_SPECULATIVE_MAP 60
#define TRIGGER_TIMEOUT_SPECULATIVE_REDUCE 10

/* Mailbox related. */
#define MAILBOX_ALIAS_SIZE 256
#define MASTER_MAILBOX "MASTER"
#define DATANODE_MAILBOX "%zu:DN"
#define TASKTRACKER_MAILBOX "%zu:TT"
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
    int            byzantine;
    int			   real_byzantine;
    msg_host_t*    workers;
    int 		   random_seed;
} config;

struct job_s {
    int           finished;
    int           tasks_pending[2];
    int*          task_instances[2]; //[phase][task_id] -> running tasks
    int*          task_replicas_instances[2]; //[phase][task_id] -> running replicas for tasks
    int*          task_confirmations[2]; //[phase][task_id] -> tasks that completed this task with the same result
    int* 		  task_byzantine_confirmations[2]; //[phase][task_id] -> byzantine nodes that gave a reply to a certain task
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

#endif /* !MRSG_COMMON_H */

// vim: set ts=8 sw=4:

