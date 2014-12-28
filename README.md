files-monitor
=============

Program that allows the user to monitor files during a specified time.


##Description
This program can be used to monitor an undetermined number of files.
If a specified word is written into any of the files, the program will output the hour it was writen and all the sentence.

######Example:

After running the program to monitor the file "file.txt" for x seconds. The word that is being monitored is "play". 

```2014-12-28T04:30:42 - file.txt - "the word play was written"```



So, the program has at least three parameters:
* The run time
* Word to monitor if was written, since the program started running
* Files to monitor

##Implementation
The program was implemented in C and uses:
* Pipes
* System calls
* Signals

##Usage

####Compilation

To compile the application run this command on the terminal:

`make`

####Run

To run the application (after compiling) just run this command:

```
./monitor time word file1 file2 [...] fileN

time - program run time
word - word to monitor if was written into any file
file1 [...] fileN - all files that the user wants to monitor
```
