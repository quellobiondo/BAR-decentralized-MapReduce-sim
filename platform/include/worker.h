

#ifndef WORKER_H
#define WORKER_H

/* hadoop-config: mapred.max.tracker.failures */
#define MAXIMUM_WORKER_FAILURES 4

typedef struct w_info_s {
	size_t  wid;
}* w_info_t;

/**
 * @brief  Get the ID of a worker.
 * @param  worker  The worker node.
 * @return The worker's ID number.
 */
size_t get_worker_id (msg_host_t worker);


#endif /* !WORKER_H */

// vim: set ts=8 sw=4:
