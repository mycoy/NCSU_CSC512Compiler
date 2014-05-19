#Compiler P3; Author: Wenzhao Zhang; UnityID: wzhang27

all: ice9

ice9: lex.yy.o ice9.tab.o ice9_log.o ice9_util.o ice9core.o ice9_print_ast.o ice9_semantic.o ice9_cg.o
	gcc -w -o ice9 lex.yy.o ice9.tab.o ice9_log.o ice9_util.o ice9_print_ast.o ice9_semantic.o ice9core.o ice9_cg.o -lfl
	
lex.yy.o: lex.yy.c ice9.tab.h
	gcc -w -c lex.yy.c
	
ice9.tab.o: ice9.tab.c ice9.tab.h ice9.h
	gcc -w -c ice9.tab.c 
	
ice9_log.o: ice9_log.c ice9_log.h
	gcc -w -c ice9_log.c 
	
ice9_util.o: ice9_util.c ice9_util.h
	gcc -w -c ice9_util.c
	
ice9core.o: ice9core.c ice9.h
	gcc -w -c ice9core.c
	
ice9_print_ast.o: ice9_print_ast.c ice9.h
	gcc -w -c ice9_print_ast.c

ice9_semantic.o: ice9_semantic.c ice9.h
	gcc -w -c ice9_semantic.c
	
ice9_cg.o: ice9_cg.c ice9.h
	gcc -w -c ice9_cg.c
	
lex.yy.c: ice9.l
	flex ice9.l
	
ice9.tab.h: ice9.y
	bison -d ice9.y
	
clean:
	rm -rf ice9 lex.yy.o ice9.tab.o ice9_log.o ice9_util.o ice9core.o ice9_print_ast.o ice9_semantic.o ice9_cg.o lex.yy.c ice9.tab.c ice9.tab.h y.output


