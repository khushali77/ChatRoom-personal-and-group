# ChatRoom-personal-and-group
This is a chatroom in C using TCP protocol in which one to one communication between two clients as well as group chat communiction(Broadcast mode) is supported.


Compile in Linux terminal using - 
gcc -pthread server_stream.c -o server
gcc -pthread client_stream.c -o server

Run using -
./server_stream <enter port number here>
./client_stream <enter port number here>
