This git repo contains my source code of the programming assignments for the course CSC512 Compiler, which is taught by Dr. Vincent W. Freeh(http://www.csc.ncsu.edu/people/vwfreeh), at the Computer Science Department of NCSU.

I get 100 for P1, 99.2 for P2, 96.5 for P3. 
The entire assignments are about making a compiler for ICE9, which is a small programming language.
P1 is lexicall and syntactical analysis, which is based on flex and bison.
P2 is semantic analysis, which is based on an AST that is crafted by myself.
P3 is code generation, and the generated code must be ran on Tiny Machine Code Simulator. 
The main-function is in ice9.y. If you only want P1, comment print_ast & semantic_check_ast & code_generation; if you want P2, comment print_ast & code_generation; if you want P3, comment print_ast; print_ast is for debugging, if you want to see the structure of the input ice9 code, uncomment this function call. 

This code is only for reference and communication. You are welcome to download, study. If you find errors or bugs, please feel free to contact me. However, you should not copy it as your own assignment submission. If you did it, you'll be caught cheating, and you should bear all the responsibility.
