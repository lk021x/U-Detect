/*
 * can_thread.h
 *
 *  Created on: 2017-1-12
 *      Author: root
 */

#ifndef  _UDETECT_THREAD_H_
#define _UDETECT_THREAD_H_

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>             /* bzero, memcpy */
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "MThread.h"

#define BUFFER_SIZE 2048

class U_DetectThread : public MThread
{
public:
	U_DetectThread();
	virtual ~U_DetectThread();

	int init_socket();
	double GetDiskFreeSpacePercent(const char *pDisk,double* freespace,double*  totalspace);
	int super_system(const char * cmd);
	int drv_write_light_gpio(const char* path, uint8_t value);
	uint32_t get_file_size(const char *path);
	void Update_From_Net();
	void run();
private:
	bool m_bStart;
	bool m_update;
	uint32_t filesize_pre, filesize_cur;
	int64_t  Time_cur, Time_last;	//当前时间，上次时间
	int CheckTimeout(uint16_t ms, int64_t &last, int64_t &cur);
};

#endif /* _CAN1_THREAD_H_ */
