# Command-Line Interpreter
A UNIX shell created as a project for an operating systems class.

My shell, called smash (for Super Madison shell) is a simple variant of a UNIX shell.
It contains several key components/functionalities of a usual shell.

# A quick note on usage
This shell expects users to specify the binary folders (namely `/bin/`) when inputting commands.
For example, **this** throws an error `smash > ls`, but **this** doesn't `smash > /bin/ls`.

# Built-in Commands
- `exit`: exits the shell when the user enters "exit"
- `cd`: changes directories using the `chdir()` system call
- `pwd`: prints the current working directory using the `*getcwd()` system call
- `loop`: loops a specified command n times, where n is the argument for the loop command (described more below)

# Redirection
- Only implemented redirection of *standard output*, using `>`
- All other forms of redirection are not supported (sorry!!)

# Multiple Commands
- Multiple commands can be added in sequence using `;` to separate the commands
- example: `cmd1 ; cmd2 args1 args2 ; cmd3 args1`, would run the commands from left to right
- Empty commands are allowed (e.g. `cmd ;` or `; cmd`)

# Pipes
- Piping on built-in commands is not explicitly supported (e.g. `pwd | /bin/grep test`)
- Redirection is supported (e.g. ` ls test.txt | /bin/grep test > output.txt`)
- Chaining pipes with semicolons is supported (e.g. `ls test.txt | /bin/grep test ; ls test.csv | /bin/grep column`)
- Chaining pipes is supported (e.g. `ls test.txt | /bin/grep test | /bin/grep test`)
- Looping on pipes is supported (e.g. `loop 5 /bin/ls | /bin/grep test`)

# Loops
- Usage: `loop 3 cmd args` runs `cmd` (with its arguments `args`) 3 consecutive times
- Looping on built-in commands is supported (e.g. `loop 4 cd ..`)
- Redirection is supported (e.g. `loop 5 cmd1 > output`), which will *rewrite the file output 5 times*
- Nested loops are not explicitly supported (e.g. `loop 5 loop 5 cmd`) (feel free to try them though, they might work!!)
- Multiple commands in loops are supported (e.g. `loop 5 cmd1 ; cmd2`), which will run `cmd1` 5 times, and then run `cmd2` once
