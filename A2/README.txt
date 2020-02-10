
Part A1
On the receiver side:
-Determine your receiver's IP
-Select a port number (between 30000 and 40000)
-Enter the following command into the terminal:
    ./receiver [port#] [failRate]
-Replace [port#] with selected port number, and [failRate] with a number
    between 0 and 100 (0 will never fail)
-Example:
    ./receiver 35790 5

On the sender side:
-Enter the following command:
    ./sender [receiverIP] [port#] [sendWindow] [timeout]
-Replace [receiverIP] with IP determined above
-Replace [port#] with same number used by receiver
-Choose a sending window size and timeout (in seconds), replace those fields
-Example:
    ./sender 128.233.236.136 35790 10 7
