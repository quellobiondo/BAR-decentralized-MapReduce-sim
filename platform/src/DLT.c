/*
 * DLT.c
 *
 *  Created on: 16 giu 2018
 *      Author: Alberto Zirondelli
 */

#include "DLT.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "worker.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);

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
	msg_task_t msg = NULL;
	DLT_block_t block = NULL;

	block = xbt_new(struct DLT_block_s, 1);
	block->original_messages = xbt_new(msg_task_t, config.block_size);
	block->original_senders = xbt_new(msg_host_t, config.block_size);

	XBT_INFO("DLT BEGIN");

	//	loop until we end the master stuff
	while (!job.finished) {
		// block for config.period_blockchain
		xbt_sleep(max(config.block_period, 0.1));

		// forward first config.blockdimension messages to master
		for(block->size = 0; block->size < config.block_size && MSG_task_listen(DLT_MAILBOX); block->size++){
			msg = NULL;
			xbt_assert(receive(&msg, DLT_MAILBOX) == MSG_OK, "ERROR RECEIVING MESSAGES");
			block->original_messages[block->size] = msg;
			block->original_senders[block->size]=MSG_task_get_source(msg);
		}

		if(block->size == 0) continue;

		task_envelope = MSG_task_create(SMS_DLT_BLOCK, 0, 0.0, (void*) block);
		xbt_assert(MSG_task_send(task_envelope, MASTER_MAILBOX) == MSG_OK, "ERROR SENDING BLOCK");
	}

	XBT_INFO("DLT END");

	xbt_free(block->original_senders);
	xbt_free(block->original_messages);
	xbt_free(block);

	return 0;
}
