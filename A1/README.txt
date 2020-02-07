/*
* Alexa Armitage ama043 11158883
* CMPT 434
* Assignment 1 Part A
*/

All parts allow the following commands to be entered into the client:
- add key value
- getvalue key
- getall
- remove key
- quit

Ports assigned:
server:32345
proxy: 34565
UDPserver: 32346
UDPproxy: 34566


To run part A1:
Open two terminal windows, and perform the commands in the following order:
On the first terminal: ./server
On the second terminal: ./client hostname(server) 32345
You can now enter commands in the client terminal


To run part A2:
Open three terminal windows, and perform the commands in the following order,
replacing hostname with the appropriate machine hostname:
On the first terminal:  ./server
On the second terminal: ./proxy hostname(server) 32345
On the third terminal:  ./client hostname(proxy) 34565
You can now enter commands in the client terminal

To run part A2:
Open three terminal windows, and perform the commands in the following order,
replacing hostname with the appropriate machine hostname:
On the first terminal:  ./server
On the second terminal: ./proxy hostname(server) 32346
On the third terminal:  ./client hostname(proxy) 34566
You can now enter commands in the client terminal
