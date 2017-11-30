/*
 * can_thread.cpp
 *
 *  Created on: 2017-1-12
 *      Author: root
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <linux/netlink.h>
#include <dirent.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include "udetect_thread.h"

#define GPIO_LIGHT_BLE					"/sys/devices/virtual/gpio/gpio68/value"

U_DetectThread::U_DetectThread()
{
	m_bStart = true;
	m_update = false;
	Time_cur = 0;
	Time_last = 0;
	filesize_pre = 0;
	filesize_cur = 0;
}

U_DetectThread::~U_DetectThread()
{

}

int U_DetectThread::init_socket()
{
    struct sockaddr_nl snl;
    const int BufferSize= 1024;
    int retval;

    memset(&snl,0,sizeof(struct sockaddr_nl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = getpid();
    snl.nl_groups = 1;

    int Sock_id = socket(PF_NETLINK,SOCK_DGRAM,NETLINK_KOBJECT_UEVENT);

    if(Sock_id == -1)
    printf("sock err:%m\n"),exit(-1);

	//int sock_flag = fcntl(Sock_id, F_GETFL, 0);
	//sock_flag = fcntl(Sock_id, F_SETFL, sock_flag | O_NONBLOCK);
	//if(sock_flag < 0) close(Sock_id),exit(-1);

    // set reveive buffer
    setsockopt(Sock_id,SOL_SOCKET,SO_RCVBUFFORCE,&BufferSize,sizeof(BufferSize));
    retval = bind(Sock_id,(struct sockaddr*)&snl,sizeof(struct sockaddr_nl));
    if(retval==-1)
    printf("bind err:%m\n"),close(Sock_id),exit(-1);
    return Sock_id;
}

// 该函数主要作用时检测u盘的 总空间,剩余空间,剩余空间百分比
double U_DetectThread::GetDiskFreeSpacePercent(const char *pDisk,double* freespace,double*  totalspace)
{
    struct statfs disk_statfs;
    double freeSpacePercent =0;
    if(statfs(pDisk,&disk_statfs) == 0)
    {

        *freespace = (disk_statfs.f_bsize * disk_statfs.f_bfree) / (1024*1024*1024.0);
        *totalspace = (disk_statfs.f_bsize * disk_statfs.f_blocks) / (1024*1024*1024.0);
    }
    return freeSpacePercent = (*freespace)/(*totalspace)*100;
}

int U_DetectThread::super_system(const char * cmd)
{
	FILE * fp;

	if (cmd == NULL) return -1;

	if ((fp = popen(cmd, "w") ) == NULL){   // "w" "r"
		printf("POPEN error: %s\n", strerror(errno));
		return -1;
	}else{
		pclose(fp);
	}
	return 0;
}

int U_DetectThread::drv_write_light_gpio(const char* path, uint8_t value)
{
	FILE * fp;
	char cmd[64]={0};

	if(value !=0 && value != 1) return -1;
	sprintf(cmd, "echo %d > %s", value, path);
	if ((fp = popen(cmd, "w") ) == NULL){   // "w" "r"
		printf("POPEN error: %s\n", strerror(errno));
		return -1;
	}else{
		pclose(fp);
	}

	return 0;
}

int U_DetectThread::CheckTimeout(uint16_t ms, int64_t &last, int64_t &cur)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	cur = (int64_t)tv.tv_sec*1000+(int64_t)tv.tv_usec/1000; //毫秒

	if(last==0 || cur<last){last=cur;return 0;}
	if(cur > last && (cur-last) >= ms){ //定时时间到
		last = cur;
		return 1;
	}else{
		return 0;
	}
}

uint32_t U_DetectThread::get_file_size(const char *path)
{
	uint32_t filesize = -1;
    struct stat statbuff;
    if(stat(path, &statbuff) < 0){
        return filesize;
    }else{
        filesize = statbuff.st_size;
    }
    return filesize;
}

void U_DetectThread::Update_From_Net()
{
	DIR *dp=NULL;
	struct dirent *ptr;
	const char*path="/opt";
	bool find = false, old = false;

	if((dp = opendir(path))!=NULL)  //打开目录成功
	{
		while((ptr=readdir(dp))!=NULL)
		{
			if(ptr->d_type == 8) //文件
			{
				char filename[128]={0};
				memcpy(filename, ptr->d_name, 256);
				if(!memcmp(filename,"update_LCD_EMS",14))   //找到包含字符串的文件, 就是升级程序
				{
					super_system("mkdir -p /opt/update 2>null");
					char rname[128]={0};
					memcpy(rname, &filename[7], 121);
					char cmd[256]={0};
					super_system("rm -r /opt/update/* 2>null");
					sprintf(cmd, "mv /opt/%s /opt/update/%s 2>null", filename, rname);
					super_system(cmd);
					find = true;
				}
			}
		}
	}
	closedir(dp);

	if(find == true)    //发现来自网络的升级程序， 替换原来的程序
	{
		if((dp = opendir(path))!=NULL)  //打开目录成功
		{
			while((ptr=readdir(dp))!=NULL)
			{
				if(ptr->d_type == 8) //文件
				{
					char filename[128]={0};
					memcpy(filename, ptr->d_name, 256);
					if(!memcmp(filename,"LCD_EMS",7))
					{
						super_system("mkdir -p /opt/bak 2>null");
						super_system("ps -ef | grep LCD | grep -v grep | cut -c 1-5 | xargs kill -9 2>null"); //终止包含“LCD”字符的进程
						super_system("rm -r /opt/bak/LCD* 2>null");
						super_system("cp -f /opt/LCD* /opt/bak 2>null");
						super_system("rm -r /opt/LCD* 2>null");
						super_system("mv -f /opt/update/* /opt/ 2>null");
					}
				}
			}
		}
		closedir(dp);
	}
	else   //没有来自网络的程序， 查找/opt下是否有程序，没有的话恢复bak中的程序
	{
		if((dp = opendir(path))!=NULL)  //打开目录成功
		{
			while((ptr=readdir(dp))!=NULL)
			{
				if(ptr->d_type == 8) //文件
				{
					char filename[128]={0};
					memcpy(filename, ptr->d_name, 256);
					if(!memcmp(filename,"LCD_EMS",7))
					{
						old = true;   //当前有程序
					}
				}
			}
		}
		closedir(dp);
		if(old == false)
		{
			super_system("cp -f /opt/bak/LCD* /opt/ 2>null");  //恢复文件
		}
	}

	super_system("./S94lcd_ems start");   //启动程序
}

void U_DetectThread::run()
{
    DIR *dp=NULL;
    struct dirent *ptr;
	char buf[BUFFER_SIZE] = {0};
	static uint8_t Run_Led = 0;
	int i=0;
	bool req_reboot = false;

	Update_From_Net();
    const char*path="/media/update";  //插上U盘后系统自动挂在在/media目录下，update是U盘下的一个目录
    int sd= init_socket();
	while(m_bStart)
	{
		recv(sd,&buf,sizeof(buf),0);   //没有数据时阻塞
		if(CheckTimeout(100, Time_cur, Time_last))  //防止升级成功重启后继续升级, 过滤上电第一次检测到u盘
		{
			if(m_update == false)
			{
				//printf("rec +++: %s\n",buf);
				if(!memcmp(buf,"add@",4) /*&& !memcmp(&buf[strlen(buf) - 4],"/sdb",4)*/)
				{
					//printf("Add U Disk!\n");
					m_update = true;
					while(i++<5)
					{
						drv_write_light_gpio(GPIO_LIGHT_BLE, Run_Led^=0x01);
						sleep(1);  //不能小于1s
						if((dp = opendir(path))!=NULL)  //u盘挂载成功后，拷贝该目录下的所有文件到 /opt/目录
						{
							//printf("打开成功!\n");
							while((ptr=readdir(dp))!=NULL)
							{
								if(ptr->d_type == 8) //文件
								{
									char filename[256]={0};
									memcpy(filename, ptr->d_name, 256);
									if(!memcmp(filename,"LCD_EMS",7))
									{
										char origename[256]={0};
										sprintf(origename, "/media/update/%s", filename);
										filesize_pre = get_file_size(origename);
										//printf("filesize_pre: %d\n", filesize_pre);
										drv_write_light_gpio(GPIO_LIGHT_BLE, 0x00);
										printf("check ok!\n");
										super_system("mkdir -p /opt/bak 2>null");
										sleep(1);
										super_system("ps -ef | grep LCD | grep -v grep | cut -c 1-5 | xargs kill -9 2>null"); //终止包含“LCD”字符的进程
										super_system("rm -r /opt/bak/LCD* 2>null");
										super_system("cp -f /opt/LCD* /opt/bak 2>null");
										super_system("cp -f /opt/eeprom /opt/bak 2>null");
										super_system("cp -f /opt/flash /opt/bak 2>null");
										//super_system("rm -rf /opt/LCD* /opt/eeprom /opt/flash 2>null"); //不要删除原来的配置文件
										super_system("rm -r /opt/LCD* 2>null");
										super_system("cp -f /media/update/* /opt/ 2>null");
										sleep(5);
										char checkname[256]={0};

										sprintf(checkname, "/opt/%s", filename);
										//printf("checkname: %s\n", checkname);
										if((access(checkname,F_OK))!=-1)
										{
											filesize_cur = get_file_size(checkname);
											if(filesize_cur == filesize_pre)
											{
												super_system("chmod 777 /opt/LCD* 2>null");
												printf("size %d, update ok!\n", filesize_cur);
											}
											else
											{
												super_system("cp -f /opt/bak/LCD* /opt/ 2>null");  //恢复文件
												printf("size %d, err!\n", filesize_cur);
											}
										}
										else
										{
											super_system("cp -f /opt/bak/LCD* /opt/ 2>null");  //恢复文件
											printf("update failed!\n");
										}
										req_reboot = true;
									}
									if(!memcmp(filename,"eeprom",6))
									{
										drv_write_light_gpio(GPIO_LIGHT_BLE, 0x00);
										sleep(1);
										super_system("ps -ef | grep LCD | grep -v grep | cut -c 1-5 | xargs kill -9 2>null"); //终止包含“LCD”字符的进程
										super_system("rm -f /opt/eeprom* 2>null");
										super_system("cp -f /media/update/eeprom /opt/ 2>null");
										sleep(1);
										req_reboot = true;
									}
									if(!memcmp(filename,"S42network0",11))
									{
										drv_write_light_gpio(GPIO_LIGHT_BLE, 0x00);
										sleep(1);
										super_system("ps -ef | grep LCD | grep -v grep | cut -c 1-5 | xargs kill -9 2>null"); //终止包含“LCD”字符的进程
										super_system("cp -f /media/update/S42network0 /etc/init.d/ 2>null");
										super_system("chmod 777 /etc/init.d/S42network0 2>null");
										sleep(1);
										req_reboot = true;
									}
									if(!memcmp(filename,"S41network1",11))
									{
										drv_write_light_gpio(GPIO_LIGHT_BLE, 0x00);
										sleep(1);
										super_system("ps -ef | grep LCD | grep -v grep | cut -c 1-5 | xargs kill -9 2>null"); //终止包含“LCD”字符的进程
										super_system("cp -f /media/update/S41network1 /etc/init.d/ 2>null");
										super_system("chmod 777 /etc/init.d/S41network1 2>null");
										sleep(1);
										req_reboot = true;
									}
								}
							}
							closedir(dp);
							dp=NULL;
							if(req_reboot == true)
							{
								req_reboot = false;
								super_system("reboot -f");
							}
						}
					}
					i=0;
				}
			}
			if(!memcmp(buf,"remove@",7))
			{
				m_update = false;
				//printf("Remove U Disk!\n");
			}
			drv_write_light_gpio(GPIO_LIGHT_BLE, 0x01);
		}
	}
}
