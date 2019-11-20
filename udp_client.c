/**
 * File              : udp_client.c
 * Author            : Ravichandra <h.ravichandra@globaledgesoft.com>
 * Date              : 20.10.2019
 * Last Modified Date: 21.11.2019
 * Last Modified By  : Ravichandra <h.ravichandra@globaledgesoft.com>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
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


struct sockaddr_in server_addr;
struct v4l2_capability caps;
struct v4l2_format format;
struct v4l2_requestbuffers buf_req;
struct v4l2_buffer buf_info;
struct v4l2_buffer buffer_info;
int fd;
void* buff;
size_t recvd, sendr;
int sock_id;
int client_id;
socklen_t length;

/*
 * \brief Function definition to initialize and set parameters
 * */
void start_device(void)
{
    int temp = 7;
    memset(&caps, 0, sizeof(caps));
    /* ioctl for querying capabilities of connected device     */
    if(ioctl(fd, VIDIOC_QUERYCAP, &caps) < 0) {
        perror("VIDIOC_QUERYCAP");
        exit(1);
    }
    /* Checking for capture capability */
    if(!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "Device does support video capture\n");
        exit(1);
    }
    if(!(caps.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "Device does support video streaming\n");
        exit(1);
    }
    /* Setting up the formats required */
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.width = 176;
    format.fmt.pix.height = 144;
    /* ioctl for setting the required format */
    if(ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        perror("VIDIOC_S_FMT");
        exit(1);
    }

    memset(&buf_req, 0, sizeof(buf_req));
    buf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf_req.memory = V4L2_MEMORY_MMAP;
    buf_req.count = 5;
    /* Structure for requesting buffers */
    if(ioctl(fd, VIDIOC_REQBUFS, &buf_req) < 0) {
        perror("VIDIOC_REQBUFS");
        exit(1);
    }

    memset(&buf_info, 0, sizeof(buf_info));
    /* Structure to store status of buffer */
    buf_info.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf_info.memory = V4L2_MEMORY_MMAP;
    buf_info.index = 0;
    if(ioctl(fd, VIDIOC_QUERYBUF, &buf_info) < 0){
        perror("VIDIOC_QUERYBUF");
        exit(1);
    }
    /* ioctl for checking status & location of buffers     */ 
/* 
    sendr = sendto(sock_id, &temp, sizeof(int),0, (struct sockaddr *)&server_addr,length);
    if(sendr == -1) {
        perror("sendto(client_side)");
        exit(1);
    }

    recvd = recvfrom(sock_id, &buf_info.length, sizeof(int), 0, (struct sockaddr *)&server_addr, &length);
    if(recvd == -1) {
        perror("reciever(my_client)");
        exit(1);
    }
*/
	buf_info.length = 50688; 
    /*Mapping descriptor with buffer */
    buff = mmap(NULL, buf_info.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf_info.m.offset);
    if(buff == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
}

/*
 * \brief Function definition to recieve & write image
 * */
void start_streaming(void)
{
    while(1) {
        memset(buff, 0, buf_info.length);
        recvd = recvfrom(sock_id, buff, buf_info.length, 0, (struct sockaddr *)&server_addr, &length);
        if(recvd == -1) {
            perror("recv(client)");
            exit(1);
        }
        printf("buf_info.length = %d\n", buf_info.length);
        write(fd, buff, buf_info.length);
    }
}

/* 
 * \brief Client program for streaming video
 * */
int main(int argc, char *argv[])
{
    char *device = "/dev/video0"; 

    /* Opening virtual node */
    fd = open(device, O_RDWR);
    if(fd < 0) {
        perror("Opening Device");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6000);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    length = sizeof(struct sockaddr_in);

    if((sock_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0){
		perror("socket");
	}
	int broadcastPermission = 1;
    if (setsockopt(sock_id, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, sizeof(broadcastPermission)) < 0){
        perror("setsockopt() failed");
    }
	if(bind(sock_id, (struct sockaddr *)&server_addr, length) == -1) {
        perror("Binding Failed\n");
        exit(EXIT_FAILURE);
    }
    start_device(); /* Function call to initialize device */
    start_streaming(); /* Function call to start capture and send */
    close(fd);
    close(client_id);
    return 0;
}
