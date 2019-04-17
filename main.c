#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <io.h>
#include <stdio.h>
#include "MQTTClient.h"
#include "MQTTPacket.h"
#include "Transport.h"
#include <string.h>
#include "cJSON.h"
#include <termios.h> 
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#define MQTT_BUFF_LEN 1400
uint8_t mqtt_buff[MQTT_BUFF_LEN] = {0};
cJSON * json_report = NULL;
char client_id[16]= {0};

int set_opt(int fd)
{	
	struct termios new_cfg = {0};
    bzero(&new_cfg, sizeof(new_cfg));
	tcgetattr(fd, &new_cfg);	
    cfmakeraw(&new_cfg);
    
	new_cfg.c_cflag |= (CLOCAL | CREAD);	

	cfsetispeed(&new_cfg, B115200);//设置波特率	
	cfsetospeed(&new_cfg, B115200);	

	new_cfg.c_cflag &= ~CSIZE;	
	new_cfg.c_cflag |= CS8;	
	new_cfg.c_cflag &= ~PARENB;	
	new_cfg.c_cflag &= ~CSTOPB;	
	new_cfg.c_cc[VTIME] = 0;	
	new_cfg.c_cc[VMIN] = 0;	

	tcflush(fd,TCIFLUSH);	
	tcsetattr(fd, TCSANOW, &new_cfg);	

	return 0;
}

int uart_init(void )
{	
    int fd = -1;
	fd = open("/dev/ttyUSB2", O_RDWR, 0);	
	if (fd < 0) 
	{		
		printf("open ttyUSB2 error!\n");		
		return -1;
	}	
	set_opt(fd);	
	return  fd;
}

////read CSQ////
//CSQ值=(接收信号强度dBm+113)/2。
void read_CSQ(void)
{
   	int ttyUSB2_Rfd = -1;
    int ttyUSB2_Wfd = -1;
	char at_csq[] = "AT+CSQ\r\n";
	int rc = 0;
    char read_buf [256] = {0};
    int i = 2;
    
    for (;i < 15; i++)
    {
        ttyUSB2_Wfd = uart_init();
        if(-1 != ttyUSB2_Wfd)
        {
            rc = write(ttyUSB2_Wfd, at_csq,strlen(at_csq));
            tcflush(ttyUSB2_Wfd, TCIFLUSH);//清空输入缓冲区
            tcflush(ttyUSB2_Wfd, TCOFLUSH);//清空输入缓冲区
            close(ttyUSB2_Wfd);

            if(rc > 0)
            {
                ttyUSB2_Rfd = open("/dev/ttyUSB2",O_RDWR);
                if(ttyUSB2_Rfd != -1) 
                {    
                    read(ttyUSB2_Rfd,read_buf,256);
                    close(ttyUSB2_Rfd);

                    if( NULL != json_report && strlen(read_buf) > 0)
                    {
                        char *ptr = strstr(read_buf,"CSQ:");
                        printf("read CSQ = %s \n",read_buf);
                        if(NULL != ptr)
                        {
                            char csq[11] = {0};
                            strncpy(csq, ptr+5, 5); 
                            if( NULL != json_report)
                            {
                                cJSON_AddItemToObject(json_report,"CSQ",cJSON_CreateString(csq));
                            }
                        }
                        return;
                    }

                } 
                else 
                {
                     printf("open /dev/ttyUSB2 failure\n");
                }
            }
            else
            {
                printf("write ttyUSB2 failure %d\n",rc);
            }

        }
        else
        {
            printf("open ttyUSB2 failure\n"); 
        }
        sleep(i/2);
    }
    
    if( NULL != json_report)
    {
        cJSON_AddItemToObject(json_report,"CSQ",cJSON_CreateString("99:99"));
    }
}


/////read client id////
void read_clientId(void)
{
	int fd = 0;
    //char clientId[16] ={0};
    char buf[256] = {0};
    
    fd = open("/dev/mtd3",O_RDWR);
      
    if(fd != -1) 
    {
        read(fd,buf,sizeof(buf)-1);
        //printf("buf = %s \n",buf);
        close(fd);
    }
    else 
    {
        printf("open /dev/mtd3 failure\n");
    }

    cJSON * json = NULL;
    json= cJSON_Parse(buf);
    if(NULL != json)
    {
        cJSON * json_cleanid= cJSON_GetObjectItem(json,"client_id");
        if(NULL != json_cleanid)
        {
            char * clientid = cJSON_Print(json_cleanid);
            if(clientid != NULL)
            {
                strncpy(client_id,clientid+1,MIN(strlen(clientid)-2,15));
                
                printf("clientId = %s \n",client_id);
                if( NULL != json_report)
                {
                    cJSON_AddItemToObject(json_report,"did",cJSON_CreateString(client_id));
                }
                free(clientid);
            }
        }
        cJSON_Delete(json);
    }
}
#define WILL_STR      "{\"did\":\"%s\"}"
#define WILL_TOPIC     "dc/cs/csq/%s"
#define MQTT_KEEP_ALIVE      60

#if 1
#define MQTT_USERNAME      "client"
#define MQTT_PASSWORD      "client@jindoo)!@!"
#endif

int mqtt_pub_station(void)
{
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    int rc = 0;
    int mysock = 0;
    MQTTString topicString = MQTTString_initializer;
    int len = 0;
    char *host = "api.jopool.net";
    int port = 1883;
    char data_clientID_cstring [20] = "";
    char will_topics_str[50] = "";
    char will_topics[50]= "";


	mysock = transport_open(host,port);
	if(mysock < 0)
		return mysock;

	//printf("Sending to hostname %s port %d\n", host, port);
    strncpy(data_clientID_cstring,client_id,15);
    strcat(data_clientID_cstring,"_csq");
    //printf("%s\n",data_clientID_cstring);
	data.clientID.cstring = data_clientID_cstring;
	data.cleansession = 1;
	data.keepAliveInterval = MQTT_KEEP_ALIVE;
	data.username.cstring = "";//MQTT_USERNAME
	data.password.cstring = "";//MQTT_PASSWORD
	data.MQTTVersion = 3;

    sprintf(will_topics_str, WILL_STR, client_id);
    sprintf(will_topics, WILL_TOPIC, client_id);

    data.willFlag = 1;
    data.will.topicName.cstring = will_topics;
    data.will.message.cstring = will_topics_str;
    data.will.qos = 2;
    data.will.retained = 0;



	len = MQTTSerialize_connect((unsigned char *)mqtt_buff, MQTT_BUFF_LEN, &data);

	topicString.cstring = will_topics;  //主题
    char * report_str = cJSON_Print(json_report); //消息
    if(NULL != report_str)
    {
	    len += MQTTSerialize_publish((unsigned char *)(mqtt_buff + len), MQTT_BUFF_LEN - len, 0, 0, 0, 0, topicString, (unsigned char *)report_str, strlen(report_str));
        free(report_str);
    }
	len += MQTTSerialize_disconnect((unsigned char *)(mqtt_buff + len), MQTT_BUFF_LEN - len);

	rc = transport_sendPacketBuffer(mysock, (unsigned char*)mqtt_buff, len);
	if (rc == len)
		printf("Successfully published\n");
	else
		printf("Publish failed\n");

    sleep(1);
    
	transport_close(mysock);

    return 0;
}

int main(int argc,char **argv)
{
    json_report = cJSON_CreateObject();
    read_clientId();
    //printf("main read_CSQ\n");
    read_CSQ();
    //printf("main mqtt_pub_station\n");
    mqtt_pub_station();
    cJSON_Delete(json_report);
    
    return 0;
}
