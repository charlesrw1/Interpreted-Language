# Interpreted-Language
A simple interpreted language in C++. The syntax looks kind of like lisp but otherwise not. It is more of an imperative language.

### How it works

The interpreter works in three steps. The first takes in a stream of characters and produces tokens, such as operators, keywords, variable names, or literals.

The next step creates a tree like structure for control logic like if statements or while loops, all steming from a root block. Each block can have multiple statements or children blocks, which lets there be nested loops and conditionals. It also finds functions at this step and puts them in a function table for calling later.

The last step is execution. I use the shunting yard algorithm for evalutating operator precedence. It basically works by pushing operators and values on to two stacks. When an operator is pushed, it checks to see if the previous one has greater precedence, and if it does, it executes the statement. When it comes on a right parentheses, it evaluates all operators till a left parentheses is seen. The program stores its local variables in a hash map, and after each block, removes the elments added in the block. The program executes recursively through each block and their children+statements. Function calls work by first creating a new enviorment with its own variables/stacks. Then the local variables are loaded, then the program executes, and then the final value is loaded back into the previous enviorment.

User defined functions: 
```
(def &func(arg1 arg2)
  (...)
  (...)
 )
(&func(1 2))
```

Callable macros in C++: 
```
(print "string")
(print (rand 1 20) " is a random number")
```
Operators that follow order of operations: 
```
(print (5+3*23/(10-3))
>> 14
```

Conditonals: 
```
(if(var1>5 && var2)(...))
(elif(var3 > 1)(...))
(else(...))
```

Variables that can be integers, strings, or lists of either: 
```
(var = 4) (num_list = 1 2 3 var 5) (my_str = "string")
```

Global variables accessible in C++: 
```
($global = "hello")
cout << *Interpreter::get_variable("global").str`
```

Loops: `(while(cond)(...))`

