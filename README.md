# Socket_File_Transfer

C Socket Programming Practice

This program is used to send file through socket in the following format:                                                              
 		./executable tcp recv <ip> <port>
 		./executable tcp send <ip> <filename>
 		./executable udp recv <ip> <port>
 		./executable udp send <ip> <filename>

Execute the program with "send" argument and specify the file which you want to send on a machine.
Then, execute the program with "recv" argument on the other machine and it will receive the file.
