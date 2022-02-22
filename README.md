# Interpreted-Language
A simple interpreted language in C++. The syntax is kind of like lisp but otherwise not.

User defined functions: `(def &func(arg1 arg2)(...)(...))` `(&func(1 2))`

Callable macros in C++: `(print "string")` `(print (rand 1 20) " is a random number") `

Conditonals: `(if(var1>5 && var2)(...))(elif(var3 > 1)(...))(else(...))`

Variables that can be integers, strings, or lists of either: `(var = 4)(num_list = 1 2 3 var 5)`

Global variables accessible in C++: `($global = "hello")` `cout << *Interpreter::get_variable("global").str`

Loops: `(while(cond)(...))`

Operators that follow order of operations: `(print (5+3*23/(10-3))`
