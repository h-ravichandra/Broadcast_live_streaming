# Broadcast live streaming
This broadcast the live stream of the connected usb camera of the system, to the systems connected to the same network

#File structure

	server.c -> Server code whih should be run in a system where usb camera is connected.
	
	udp_client.c -> Client code which should be run in all systems where all we want to see the strea of data in the vlc.

#Setup steps to be followed

	1. Connect a usb camera to the server system,
	2. Configure inet_ddr of server code to your brodcast Ip,
	3. git clone https://github.com/umlaeute/v4l2loopback.git
	4. After cloning the v4l2loopback follow the README.md of v4l2loopback to create the virtual video0 device file in your client machines,
	5. Check both server and client code that you are opening the correct device file (video0 or video1) in your system,
	
#Execution steps

	Client:
		gcc -o client udp_client.c
		./client
		To see the live stream (open vlc -> media -> open capture device -> video device name -> (select correct device file (/dev/video0 or /dev/video1))
		
	Server:
		gcc -o server server.c
		./server
		(This will start sending the stream to the all client which are running successfully).

#Useful link
http://jwhsmith.net/2014/12/capturing-a-webcam-stream-using-v4l2/

