#include "udetect_thread.h"

U_DetectThread detect;

//调试前使用下面的命令挂载本地NFS目录
//mount -t nfs 192.168.1.128:/nfs_root /mnt/nfs_root/ -o nolock

int main()  
{  
	detect.start();
	detect.wait();
    return 0;  
}
