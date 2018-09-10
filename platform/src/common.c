

#include "common.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);

msg_error_t send(const char* str, double cpu, double net, void* data,
		const char* mailbox) {
	msg_error_t status;
	msg_task_t msg = NULL;

	msg = MSG_task_create(str, cpu, net, data);

#ifdef VERBOSE
	if (!message_is (msg, SMS_HEARTBEAT))
	XBT_INFO ("TX (%s): %s", mailbox, str);
#endif

	status = MSG_task_send(msg, mailbox);

#ifdef VERBOSE
	if (status != MSG_OK)
	XBT_INFO ("ERROR %d SENDING MESSAGE: %s", status, str);
#endif

	return status;
}

msg_error_t send_sms(const char* str, const char* mailbox) {
	return send(str, 0.0, 0.0, NULL, mailbox);
}

msg_error_t receive(msg_task_t* msg, const char* mailbox) {
	msg_error_t status;

	status = MSG_task_receive(msg, mailbox);

#ifdef VERBOSE
	if (status != MSG_OK)
	XBT_INFO ("ERROR %d RECEIVING MESSAGE", status);
#endif

	return status;
}

msg_error_t receiveTimeout(msg_task_t* msg, const char* mailbox, double timeout) {
	msg_error_t status;
	status = MSG_task_receive_with_timeout(msg, mailbox, timeout);

#ifdef VERBOSE
	if (status != MSG_OK)
	XBT_INFO ("ERROR %d RECEIVING MESSAGE", status);
#endif

	return status;
}

int message_is(msg_task_t msg, const char* str) {
	if (strcmp(MSG_task_get_name(msg), str) == 0)
		return TRUE;

	return FALSE;
}

int maxval(int a, int b) {
	if (b > a)
		return b;

	return a;
}

/**
 * @brief  Return the output size of a map task.
 * @param  mid  The map task ID.
 * @return The task output size in bytes.
 */
size_t map_output_size(size_t mid) {
	size_t rid;
	size_t sum = 0;

	for (rid = 0; rid < config.amount_of_tasks[REDUCE]; rid++) {
		sum += user.map_output_f(mid, rid);
	}

	return sum;
}

/**
 * @brief  Return the input size of a reduce task.
 * @param  rid  The reduce task ID.
 * @return The task input size in bytes.
 */
size_t reduce_input_size(size_t rid) {
	size_t mid;
	size_t sum = 0;

	for (mid = 0; mid < config.amount_of_tasks[MAP]; mid++) {
		sum += user.map_output_f(mid, rid);
	}

	return sum;
}

/*
 * Tell how many replicas do you want for every task
 * The replicas are considered as different nodes
 * The replicas are not to solve the stragglers (mainly)
 *
 * Improvement idea: consider also the failures like in MOON?
 */
int number_of_task_replicas(){
	int necessary_replicas_to_be_BFT = config.byzantine + 1;
	return min(config.number_of_workers, necessary_replicas_to_be_BFT);
}

// vim: set ts=8 sw=4:
