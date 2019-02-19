#FileCopy 

In this program, the contents of one input file is copied to another output on the same host.

#SimpleFileTransfer 

This simulates a client server architecture. The server is started on a specified port number. The client program are given the following command line arguments:
	1. Input file name: File from which the contents are to be copied.
	2. Output file name: The file is opened by the server and the data is copied to this file.
	3. Server IP address: This lets the client know the address of the server in the network.
	4. Port number: Specifying the port number on which the service for the server is running. 

This is a best-effort implementation and we assume that connection is made over a reliable channel. There is no provisioning error detection of packet loss.

#RDT2.2

This extends the SimpleFileTransfer implementation with additional features of error detection. For error detection, checksum field is added to the header field of the packet. To make sure that all the packets are received to the server an ACK system is used. ACK messages allow the clients to know whether the data received and had no errors. However, in this implementation we still assume that the channel through which the communication is made is  reliable.

#RDT3.0

This extends the RDT2.2 implementation which allows communication over unreliable channel as well. To guarantee the complete transfer of data a timer feature is added to the client side. If the timer expires before an ACK message is received then the client resends the packet. The timer is started for each and ever packet.