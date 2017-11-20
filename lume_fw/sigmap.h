/*
 * sigmap.h
 *
 *  Created on: 13 ????. 2017 ?.
 *      Author: juice
 */

#ifndef SIGMAP_H_
#define SIGMAP_H_

#define SIG_COUNT 8
#define SIG_OFFSET 2

const uint8_t SIG_MAP[SIG_COUNT] =  {

		TICK_SEC_SIG,
		SHELL_COMMAND_SIG,
		LUM_CHANGED_SIG,
		BTN_PLUS_SIG,
		BTN_MINUS_SIG,
		BTN_DOWN_SIG,
		BTN_UP_SIG,
		TIMEOUT_SIG,
};

#endif /* SIGMAP_H_ */
