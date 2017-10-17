#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include "qxwz_rtcm.h"
#include <string.h>
//#include "qxlog.h"

#undef QXLOGI
#define QXLOGI printf
int fdSerial=0;
char* portname="/dev/ttyS0";
//#define _QXWZ_TEST_START_STOP

qxwz_account_info *p_account_info = NULL;
void  get_qxwz_sdk_account_info(void);

void qxwz_rtcm_response_callback(qxwz_rtcm data){
    printf("QXWZ_RTCM_DATA:%s\n",data.buffer);
    QXLOGI("QXWZ_RTCM_DATA:%s\n",data.buffer);
    QXLOGI("QXWZ_RTCM_size:%ld\n",data.length);
}

void qxwz_status_response_callback(qxwz_rtcm_status code){
    //printf("QXWZ_RTCM_STATUS:%d\n",code);
    QXLOGI("QXWZ_RTCM_STATUS:%d\n",code);
	struct tm *ptr = NULL;
	//test account expire
	if(code == QXWZ_STATUS_OPENAPI_ACCOUNT_TOEXPIRE){
		get_qxwz_sdk_account_info();
	}
}

void  get_qxwz_sdk_account_info(void)
{
	p_account_info = getqxwzAccount();
	if(p_account_info->appkey != NULL) {
		printf("appkey=%s\n",p_account_info->appkey);
	}
	if(p_account_info->deviceID != NULL) {
		printf("deviceID=%s\n",p_account_info->deviceID);
	}
	if(p_account_info->deviceType != NULL) {
		printf("deviceType=%s\n",p_account_info->deviceType);
	}

	if(p_account_info->NtripUserName != NULL) {
		printf("NtripUserName=%s\n",p_account_info->NtripUserName);
	}
	if(p_account_info->NtripPassword != NULL) {
		printf("NtripPassword=%s\n",p_account_info->NtripPassword);
	}
	printf("expire_time=%d\n",p_account_info->expire_time);
}



//void getAccountExpireDate(void);


#ifdef _QXWZ_TEST_START_STOP
pthread_t qxwz_rtcm_test;
void test_qxwz_rtcm_start_stop(void);
#endif

int setOpt(int fd,int nSpeed,int nBits,char nEvent,int nStop)
{
	struct termios newtio,oldtio;
	if(tcgetattr(fd,&oldtio)!=0)
	{
		perror("SetupSerial 1");
	}
	bzero(&newtio,sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	switch(nBits)
	{
		case 7:
				newtio.c_cflag |= CS7;
				break;
		case 8:
				newtio.c_cflag |= CS8;
				break;
	}
	switch (nEvent)
	{
		case 'O':
				newtio.c_cflag |= PARENB;
				newtio.c_cflag |= PARODD;
				newtio.c_cflag |= (INPCK | ISTRIP);
				break;
		case 'E':
				newtio.c_iflag |= (INPCK | ISTRIP);
				newtio.c_cflag |= PARENB;
				newtio.c_cflag |= ~PARODD;
				break;
		case 'N':
				newtio.c_cflag &= ~PARENB;
				break;
	}
	switch(nSpeed)
	{
		case 9600:
				cfsetispeed(&newtio,B9600);
				cfsetospeed(&newtio,B9600);
				break;
		case 115200:
				cfsetispeed(&newtio,B115200);
				cfsetospeed(&newtio,B115200);
				break;
		default:
				cfsetispeed(&newtio,B9600);
				cfsetospeed(&newtio,B9600);
				break;
	}
	if(nStop == 1)
	{
		newtio.c_cflag &= ~CSTOPB;
	}
	else if(nStop ==2)
	{
		newtio.c_cflag |= CSTOPB;
	}
	newtio.c_cc[VTIME]= 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(fd,TCIFLUSH);
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("com set error");
		return -1;
	}
	printf("set done!\n");
	return 0;
}

int  sendDataTty(int fd,char *send_buf,int len)
{
	ssize_t ret;

	ret = write(fd,send_buf,len);
	if( ret == -1)
	{
		printf("write device error");
		return -1;
	}
	
	return 1;
}	

int OpenPort(int fd,char* portname)
{
	fd=open(portname,O_RDWR | O_NOCTTY | O_NDELAY);
	if(-1 == fd)
	{
		perror("Can't open serial port");
		return -1;
	}
}

int  SendComandToUart1( char *buf)
{
	ssize_t ret;
	if(fdSerial==0)
			return -1;
	ret = write(fdSerial,buf,sizeof(buf));
	if(ret == -1)
	{
		printf("write data error");
		return -1;
	}
	return 1;
}

int  SerialInit()
{
	int iSetOpt =0;
	int fdSerial =0;
	if(fdSerial = OpenPort(fdSerial,portname))
	{
		perror("open_port error");
		return -1;
	}
	if((iSetOpt = setOpt(fdSerial,9600,8,'N',1))<0)
	{
		perror("set_opt error");
		return -1;
	}
	tcflush(fdSerial,TCIOFLUSH);//清掉串口缓存
	fcntl(fdSerial,F_SETFL,0);

}



void SendComandAskGGAFromUart1()
{
	SendComandToUart1("assignall all auto");//enable all gps and beidou
	SendComandToUart1("log [com1] gpgga ontime 0.2");//ask uart1 output GGA data 5hz
	SendComandToUart1("interfacemode com2 rtcm none off");//ask  uart2 input rtcm data
}


int main(int argc, const char * argv[]) {
    //设置appKey和appSecret
    //apapKey申请详细见说明文档
    qxwz_config config;
    //RTD	
    config.appkey="522247";
    config.appSecret="5f8cd1c530f077cc855f487b9bfded1c959ba34f54d6043ddc1989363b43e9bc";
    config.deviceId="12345";
    config.deviceType="123456";

    qxwz_setting(&config);
    //启动rtcm sdk
    qxwz_rtcm_start(qxwz_rtcm_response_callback,qxwz_status_response_callback);

	#ifdef _QXWZ_TEST_START_STOP
    pthread_create(&qxwz_rtcm_test,NULL,test_qxwz_rtcm_start_stop,NULL);
	#endif
    //demo测试10秒发送gga
    //每秒发送gga以获取最新的rtcm数据流
    int i;
    for (i = 0; i < 120; i++) {
        qxwz_rtcm_sendGGAWithGGAString("$GPGGA,000001,3112.518576,N,12127.901251,E,1,8,1,0,M,-32,M,3,0*4B\r\n");
        //printf("Send GGA done\r\n");
        QXLOGI("Send GGA done\r\n");
		//getAccountExpireDate();
		get_qxwz_sdk_account_info();
        sleep(1);
    }
    QXLOGI("qxwz_rtcm_stop here\r\n");
//    //关闭rtcm sdk
    qxwz_rtcm_stop();
    QXLOGI("qxwz_rtcm_stop done\r\n");
    return 0;
}
#if 0
void getAccountExpireDate(void)
{
	struct tm *ptr = NULL;
	expire_time = qxwz_get_account_expire_time();
	//printf("expire_time=%d,date=",expire_time.expire_time);
	QXLOGI("expire_time=%d,date=",expire_time.expire_time);
	ptr = &expire_time.expire_date;
		
	QXLOGI("year:%d,month:%d,mday:%d,hour:%d,minute:%d,second:%d\n", \
		ptr->tm_year+1900,ptr->tm_mon+1,ptr->tm_mday,ptr->tm_hour,ptr->tm_min,ptr->tm_sec);
}
#endif

#ifdef _QXWZ_TEST_START_STOP
void test_qxwz_rtcm_start_stop(void)
{
	//sleep(2);
	qxwz_rtcm_stop();
    while(1)
    {
    	qxwz_rtcm_start(qxwz_rtcm_response_callback,qxwz_status_response_callback); 
		sleep(50);
	    time_t time_stop_begin = time(NULL);
	    qxwz_rtcm_stop();
	    time_t time_stop_end = time(NULL);
	    QXLOGI("time_stop_begin:%d,time_stop_end:%d\n",time_stop_begin,time_stop_end);
		sleep(1);
    }
}
#endif


