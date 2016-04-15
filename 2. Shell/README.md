README

wsh - a simple shell written by Dan Kenefick

Built in commands
help: displays the built in commands
cd (path) : Changes the path. An empty cd changes to the home directory. 
~ is shorthand for your home diectory too.
jobs: a list of currently executing background jobs and their job number (pid)
kill [-a] (job number): kill the specified job. the -a flag kills all processes.
exit: kill all child processes, exit the shell
bg (job number): allow the stopped job specified by job number to run in the background

Extras implemented:

&& - test linking: second command is only evaluated if the first exits successfully
>> - appending output: append a file rather than write over it
ctrl-z - SIGTSTP: stop the foreground process
bg - background a process: allow a stopped process to continue quietly in the background

noticably lacking:
fg - not enough time to implement



