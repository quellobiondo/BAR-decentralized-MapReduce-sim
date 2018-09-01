#include "DLT.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "worker.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);

typedef struct {
	msg_task_t msg;
	msg_host_t source;
} tx;

tx pool_tx[MAX_SIZE_POOL_TX];
int pool_size = 0;

int TrasactionPoolWorker(int argc, char *argv[]){
	tx new_tx;
	int pool_index = 0;

	while(!job.finished){
		// forward first config.blockdimension messages to master
		for(; MSG_task_listen(DLT_MAILBOX) && pool_size < MAX_SIZE_POOL_TX; pool_size++){
			new_tx.msg = NULL;
			xbt_assert(receive(&new_tx.msg, DLT_MAILBOX) == MSG_OK, "ERROR RECEIVING MESSAGES");
			new_tx.source = MSG_task_get_source(new_tx.msg);
			pool_tx[pool_index] = new_tx;
			pool_index = (pool_index+1) % MAX_SIZE_POOL_TX;
		}
		MSG_process_sleep(1);
	}
	return 0;
}

/**
 * @brief  Main DLT function.
 * this is the main function for the DLT node, it loops to receive messages, then it waits for
 * a timeout due to create the block, and then forward the messages that can be in the block to the schedulers
 * that will take a decision
 *
 * Ideally, this would have been on all the nodes, decentralized, but a centralized approach like this emulate
 * the behavior with good approximation for our latency, in which the biggest cost will be the computation time
 * of map, reduce, the shuffling phase and the time for block creation.
 */
int DLT(int argc, char *argv[]){
	msg_task_t task_envelope = NULL;
	DLT_block_t block = NULL;
	int tx_aggregated = 0;
	int transferred_index = 0;
	msg_error_t status;
	const char* hostname = MSG_host_get_name(MSG_host_self());


	xbt_assert(MAX_SIZE_POOL_TX > config.block_size);

	/* Spawn a process that listens for tasks. */
	MSG_process_create("tx-aggregator", TrasactionPoolWorker, NULL, MSG_host_self());

	TRACE_host_state_declare("BLOCK");
	TRACE_host_state_declare_value("BLOCK", "COMPUTATION", "0.7 0.7 0.7");
	TRACE_host_state_declare_value("BLOCK", "AGGREGATION", "0.1 0.7 0.1");
	TRACE_host_variable_declare_with_color("TRANSACTIONS", "0.8 0.2 0.2");

	XBT_INFO("DLT BEGIN");

	//	loop until we end the master stuff
	while (!job.finished) {

		tx_aggregated = pool_size;
		XBT_VERB ("INFO sending %d transactions aggregated", tx_aggregated);

		TRACE_host_set_state(hostname, "BLOCK", "AGGREGATION");
		block = xbt_new(struct DLT_block_s, 1);
		block->original_messages = xbt_new(msg_task_t, config.block_size);
		block->original_senders = xbt_new(msg_host_t, config.block_size);

		// forward first config.blockdimension messages to master
		for(block->size = 0; block->size < min(config.block_size, tx_aggregated); block->size++){
			block->original_messages[block->size] = pool_tx[transferred_index].msg;
			block->original_senders[block->size]= pool_tx[transferred_index].source;
			transferred_index = (transferred_index+1) %  MAX_SIZE_POOL_TX;
		}

		pool_size -= block->size; //remove from the tx pool the transactions sent

		TRACE_host_set_state(hostname, "BLOCK", "COMPUTATION");

		XBT_VERB ("INFO Computing time to create a BLOCK");
		// block for config.period_blockchain
		MSG_process_sleep(max(config.block_period, 0.1));

		if(block->size == 0) continue;


		XBT_VERB ("Sending BLOCK with %d txs", block->size);


		TRACE_host_variable_set(hostname, "TRANSACTIONS", block->size);

		task_envelope = MSG_task_create(SMS_DLT_BLOCK, 0, 0.0, (void*) block);
		status = MSG_task_send(task_envelope, MASTER_MAILBOX);
		if(status != MSG_OK){
			XBT_CRITICAL("STatus of the message != OK -> %d", status);
			xbt_assert(status == MSG_OK);
		}
	}

	XBT_INFO("DLT END");

	xbt_free(block->original_senders);
	xbt_free(block->original_messages);
	xbt_free(block);

	return 0;
}
