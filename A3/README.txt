/*
* Alexa Armitage ama043 11158883
* CMPT 434
* Assignment 3 Part A
*/

How to start a router:
router [name] [myPort] [theirPort] [optionalPort]

Some sample commands for setting up an a-b-c chain
router C 38451 38350
router E 38662 38451
router G 38350 38351
If you kill E or G, you will get a count-to-infinity scenario.

This will test the optional port.
router C 38451 38349
router E 38662 38451 38350
router G 38350 38351
