# Command-Line Interpreter
A UNIX shell created as a project for an operating systems class.

My shell, called smash (for Super Madison shell) is a simple variant of a UNIX shell.
It contains several key components/functionalities of a usual shell.

# Built-in Commands
- `exit`: exits the shell when the user enters "exit"
- `cd`: changes directories using the `chdir()` system call
- `pwd`: prints the current working directory using the `*getcwd()` system call
- `loop`: loops a specified command n times, where n is the argument for the loop command

# Redirection
- Only implemented redirection of *standard output*, using `>`
- All other forms of redirection are not supported (sorry!!)

# Multiple Commands
- Multiple commands can be added in sequence using `;` to separate the commands
- example: `smash> cmd1 ; cmd2 args1 args2 ; cmd3 args1`, would run the commands from left to right

# Pipes
- Piping on built-in commands is not explicitly supported (e.g. `pwd | /bin/grep test`)
- Redirection is supported (e.g. ` ls test.txt | /bin/grep test > output.txt`)
- Chaining pipes with semicolons is supported (e.g. `ls test.txt | /bin/grep test ; ls test.csv | /bin/grep column`)
- Chaining pipes is supported (e.g. `ls test.txt | /bin/grep test | /bin/grep test`)
- Looping on pipes is supported (e.g. `loop 5 /bin/ls | /bin/grep test`)
