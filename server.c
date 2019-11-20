/**
 * File              : server.c
 * Author            : Ravichandra <h.ravichandra@globaledgesoft.com>
 * Date              : 20.10.2019
 * Last Modified Date: 21.11.2019
 * Last Modified By  : Ravichandra <h.ravichandra@globaledgesoft.com>
 */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <netdb.h>
#include <time.h>

struct stream {
    void *start;
    int length;
};
int sock_id;
size_t sendr;   
struct v4l2_capability caps;
struct v4l2_format format;
struct v4l2_requestbuffers buf_req;
struct v4l2_buffer buf_info;
struct v4l2_buffer buffer_info;
int fd;
int iter;
struct stream *buffer;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;
socklen_t length;

/*
 * \brief Function definition to initialize and set parameters
 * */
void start_device(void)
{
    memset(&caps, 0, sizeof(caps));
    /* ioctl for querying capabilities of connected device */
    if(ioctl(fd, VIDIOC_QUERYCAP, &caps) < 0) {
        perror("VIDIOC_QUERYCAP");
        exit(1);
    }
    /* Checking for capture capability */
    if(!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "Device does support video capture\n");
        exit(1);
    }
    /* Checking for capture capability */
    if(!(caps.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "Device does support video streaming\n");
        exit(1);
    }
    memset(&format, 0, sizeof(format));
    /* Checking for capture capability */
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.width = 176;
    format.fmt.pix.height = 144;
    /* Checking for capture capability */
    if(ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        perror("VIDIOC_S_FMT");
        exit(1);
    }
    /* Structure for requesting buffers */
    memset(&buf_req, 0, sizeof(buf_req));
    buf_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf_req.memory = V4L2_MEMORY_MMAP;
    buf_req.count = 5;
    /* ioctl for requesting buffers */
    if(ioctl(fd, VIDIOC_REQBUFS, &buf_req) < 0) {
        perror("VIDIOC_REQBUFS");
        exit(1);
    }

    memset(&buf_info, 0, sizeof(buf_info));
    /* Allocating memory for user buffer */
    buffer =  calloc(buf_req.count, sizeof(struct stream));

    for(iter = 0; iter < buf_req.count; iter++) {
        /* Structure to store status of buffer */
        buf_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf_info.memory = V4L2_MEMORY_MMAP;
        buf_info.index = iter;
        /* Structure to store status of buffer */
        if(ioctl(fd, VIDIOC_QUERYBUF, &buf_info) < 0){
            perror("VIDIOC_QUERYBUF");
            exit(1);
        }
        buffer[iter].length = buf_info.length;
        /*Mapping descriptor with buffer */
        buffer[iter].start = mmap(NULL, buf_info.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf_info.m.offset);
        if(buffer[iter].start == MAP_FAILED)
        {
            perror("mmap");
            exit(1);
        }
    }
}
/*
 * \brief Function definition to capture & send image through socket
 * */
void start_capture(void)
{

    int type;
    int validate;
    int temp;
/*
	printf("ready to recieve\n");
    validate = recvfrom(sock_id, &temp, sizeof(int), 0, (struct sockaddr *)&server_addr, &length);
    if (validate == -1) {
        perror("recvfrom(server_side)");
        exit(1);
    }
	printf("\\");
*/
    memset(&buffer_info, 0, sizeof(buffer_info));
    /* Loop for iterating buffers */
    for(iter = 0; iter < buf_req.count; iter++) {
        buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer_info.memory = V4L2_MEMORY_MMAP;
        buffer_info.index = iter;

        if(ioctl(fd, VIDIOC_QBUF, &buffer_info) < 0) {
            perror("VIDIOC_QBUF");
            exit(1);
        }
    }
    type = buffer_info.type;
    /* ioctl to start streaming */
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("VIDIOC_STREAMON");
        exit(1);
    }
    sleep(1);
    /* Sending buffer size through socket */
/*
    sendr = sendto(sock_id, &buffer_info.length, sizeof(int), 0,(struct sockaddr *)&server_addr, length);
    if(sendr == -1) {
        perror("send(server1)");
        exit(1);
    }
*/
    while(1) {
        for(iter = 0; iter < buf_req.count; iter++) {
            /* ioctl for dequeying filled buffer */
            if(ioctl(fd, VIDIOC_DQBUF, &buffer_info) < 0){
                perror("VIDIOC_QBUF");
                exit(1);
            }
            printf("buffer_info.length = %d\n", buffer_info.length);
            /* Sending frame over socket */
            sendr = sendto(sock_id, buffer[iter].start, buffer_info.length, 0,(struct sockaddr *)&server_addr, length);
            if(sendr == -1) {
                perror("send(server2)");
                exit(1);
            }
            printf("send bytes= %ld\n",sendr);
            memset(buffer[iter].start, 0, buffer_info.length);
        }
        for(iter = 0; iter < buf_req.count; iter++) {
            buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer_info.memory = V4L2_MEMORY_MMAP;
            buffer_info.index = iter;

            if(ioctl(fd, VIDIOC_QBUF, &buffer_info) < 0) {
                perror("VIDIOC_QBUF");
                exit(1);
            }
        }
    }
    /* ioctl to terminate streaming */
    if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        perror("VIDIOC_STREAMOFF");
        exit(1);
    }
}

int main(int argc,char *argv[])
{
    char *device = "/dev/video1"; /* Camera device node */   
    int binder;
    /* Creating structure for socket */                                                        
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6000);
    server_addr.sin_addr.s_addr = inet_addr("192.168.2.255");//Brocast Ip of your network
    length = sizeof(struct sockaddr_in);

    if((sock_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0){
		perror("socket");
	} /* Creating socket */
    /* Binding the socket with the structure */
/*    binder = bind(sock_id, (struct sockaddr *)&server_addr, length);
    if(binder < 0) {
        perror("binder");
        return 0;
    }
*/
	int broadcastPermission = 1;
    if (setsockopt(sock_id, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, sizeof(broadcastPermission)) < 0){
        perror("setsockopt() failed");
    }

    /* Opening camera device node */
    fd = open(device, O_RDWR);
    if(fd < 0) {
        perror("Opening Device");
        return 0;
    }
    printf("Device opened successfully\n");

    start_device();   /* Function call to initialize device */
    start_capture();  /* Function call to start capture and send */
    close(sock_id);
    close(fd);
    return 0;
}
