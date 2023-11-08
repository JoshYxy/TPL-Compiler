%{
#include <stdio.h>
#include <string.h>
#include "TeaplAst.h"
#include "y.tab.hpp"
extern int line, col;
extern void set_pos(int cnt);
extern A_pos get_pos(); 
extern void set_num();
extern void set_id();
%}


%x COMMENT1
%x COMMENT2

%%

"//" { BEGIN COMMENT1; }

<COMMENT1>"\n" { line++; col = 1; BEGIN 0; }
<COMMENT1>[^\n]*


"/*" { BEGIN COMMENT2; } 

<COMMENT2>"\n" { line++; col=1; } 
<COMMENT2>"*/" { BEGIN 0; }
<COMMENT2>.

if { set_pos(2); return IF; } 
while { set_pos(5); return WHILE; } 
else { set_pos(4); return ELSE; } 
break { set_pos(5); return BREAK; } 
continue { set_pos(8); return CONTINUE; } 
ret { set_pos(3); return RET; } 
fn { set_pos(2); return FN; } 
let { set_pos(3); return LET; } 
struct { set_pos(6); return STRUCT; }

int { set_pos(3); return INT; }

";"  { set_pos(1); return SEMICOLON; }
"="  { set_pos(1); return ASSIGN; }
"."  { set_pos(1); return DOT; }
":"  { set_pos(1); return COLON; }
","  { set_pos(1); return COMMA; }
"->" { set_pos(2); return ARROW; }

"+"	{ set_pos(1); return ADD; }
"-"	{ set_pos(1); return SUB; }
"*"	{ set_pos(1); return MUL; }
"/"	{ set_pos(1); return DIV; }

">=" { set_pos(2); return UNLESS; }
"<=" { set_pos(2); return UNGREATER; }
"==" { set_pos(2); return EQUAL; }
"!=" { set_pos(2); return UNEQUAL; }
">" { set_pos(1); return GREATER; }
"<" { set_pos(1); return LESS; }
"!" { set_pos(1); return NOT; }

"&&" { set_pos(2); return AND; }
"||" { set_pos(2); return OR; }


"("	{ set_pos(1); return LPAR; }
")"	{ set_pos(1); return RPAR; }
"["	{ set_pos(1); return LBKT; }
"]"	{ set_pos(1); return RBKT; }
"{"	{ set_pos(1); return LBRA; }
"}"	{ set_pos(1); return RBRA; }


[1-9][0-9]*|"0" { set_num(); return NUM; }

[a-zA-Z][a-zA-Z0-9]* { set_id(); return ID; }

[\t] { col+=4; }
[ ] { col++; }

"\n" { line++; col = 1;}
. { col++; }

%%
void set_pos(int cnt) 
{
    //free(yylval.pos);
    yylval.pos = (A_pos)malloc(sizeof(*yylval.pos));

    yylval.pos -> line = line;
    yylval.pos -> col = col;
    col += cnt;
}
void set_num() {
    // if(yylval.exprUnit && yylval.exprUnit->pos) free(yylval.exprUnit->pos);
    //free(yylval.exprUnit);
    yylval.exprUnit = (A_exprUnit)malloc(sizeof(*yylval.exprUnit));
    yylval.exprUnit->pos = (A_pos)malloc(sizeof(*yylval.pos));
    yylval.exprUnit->u.num = atoi(yytext); 
    yylval.exprUnit->kind = A_numExprKind;
    yylval.exprUnit->pos->line = line;
    yylval.exprUnit->pos->col = col;
    col += yyleng;
}
void set_id() {
   // if(yylval.exprUnit && yylval.exprUnit->pos) free(yylval.exprUnit->pos);
    //free(yylval.exprUnit);
    yylval.exprUnit = (A_exprUnit)malloc(sizeof(*yylval.exprUnit));
    yylval.exprUnit->pos = (A_pos)malloc(sizeof(*yylval.pos));
    yylval.exprUnit->u.id = (char*)malloc(yyleng);
    //yylval.exprUnit->u.id = new std::string(yytext); 
    strcpy(yylval.exprUnit->u.id, yytext);
    
    yylval.exprUnit->kind = A_idExprKind;
    yylval.exprUnit->pos->line = line;
    yylval.exprUnit->pos->col = col;
    col += yyleng;
}
A_pos get_pos() 
{
    A_pos p = (A_pos)malloc(sizeof(*p));
    p->line = line;
    p->col = col;

    return p;
}

