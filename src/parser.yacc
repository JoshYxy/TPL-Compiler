%{
#include <stdio.h>
#include <cstdlib>
#include "TeaplAst.h"

extern A_pos pos;
extern A_program root;

extern int yylex(void);
extern "C"{
extern void yyerror(char *s); 
extern int  yywrap();
}

%}

// TODO:
// your parser

%union {
  A_pos pos;
  A_program program;
  A_programElementList programElementList;
  A_programElement programElement;
  A_arithExpr arithExpr;
  A_exprUnit exprUnit;
  A_structDef structDef;
  A_varDeclStmt varDeclStmt;
  A_fnDeclStmt fnDeclStmt;
  A_fnDef fnDef;

  A_rightValList rightValList;
  A_rightVal rightVal;
  A_leftVal leftVal;
  A_boolExpr boolExpr;
  A_boolUnit boolUnit; 
  A_varDeclList varDeclList;
  A_varDecl varDecl;
  A_varDef varDef;
  A_type type;
  A_fnDecl fnDecl;
  A_paramDecl paramDecl;

  A_codeBlockStmtList codeBlockStmtList;
  A_codeBlockStmt codeBlockStmt;
  A_returnStmt returnStmt;
  A_callStmt callStmt;
  A_fnCall fnCall;
  A_assignStmt assignStmt;
  A_ifStmt ifStmt;
  A_whileStmt whileStmt;
}

%token <pos> ADD
%token <pos> SUB
%token <pos> MUL
%token <pos> DIV

%token <pos> SEMICOLON // ;
%token <pos> ASSIGN // =
%token <pos> DOT    // .
%token <pos> COLON  // :
%token <pos> COMMA // ,
%token <pos> ARROW // ->

%token <pos> UNLESS  // >=
%token <pos> UNGREATER // <=
%token <pos> EQUAL  // ==
%token <pos> UNEQUAL  // !=
%token <pos> GREATER  // >
%token <pos> LESS  // <
%token <pos> NOT  // !

%token <pos> AND  // &&
%token <pos> OR  // ||

%token <pos> LPAR  // (
%token <pos> RPAR  // )
%token <pos> LBKT  // [
%token <pos> RBKT  // ]
%token <pos> LBRA  // {
%token <pos> RBRA  // }

%token <pos> LET
%token <pos> IF
%token <pos> ELSE
%token <pos> WHILE
%token <pos> BREAK
%token <pos> CONTINUE
%token <pos> RET
%token <pos> FN
%token <pos> STRUCT

%token <pos> INT

%token <exprUnit> NUM //may be improved
%token <exprUnit> ID

%left ADD SUB
%left MUL DIV
%left OR
%left AND
%nonassoc	UMINUS


%type <program> Program
%type <arithExpr> ArithExpr
%type <programElementList> ProgramElementList
%type <programElement> ProgramElement
%type <exprUnit> ExprUnit
%type <structDef> StructDef
%type <varDeclStmt> VarDeclStmt
%type <fnDef> FnDef
%type <fnDeclStmt> FnDeclStmt


%type <rightVal> RightVal
%type <rightValList> RightValList
%type <leftVal> LeftVal
%type <boolExpr> BoolExpr
%type <boolUnit> BoolUnit
%type <varDeclList> VarDeclList
%type <varDecl> VarDecl
%type <varDef> VarDef
%type <type> Type
%type <fnDecl> FnDecl
%type <paramDecl> ParamDecl;

%type <codeBlockStmtList> CodeBlockStmtList;
%type <codeBlockStmt> CodeBlockStmt;
%type <returnStmt> ReturnStmt;
%type <callStmt> CallStmt;
%type <fnCall> FnCall;
%type <assignStmt> AssignStmt;
%type <ifStmt> IfStmt;
%type <whileStmt> WhileStmt;

%start Program

%%                   /* beginning of rules section */

Program: ProgramElementList 
{  
  root = A_Program($1);
  $$ = A_Program($1);
}
;

ProgramElementList: ProgramElement ProgramElementList
{
  $$ = A_ProgramElementList($1, $2);
}
|
{
  $$ = NULL;
}
;

ProgramElement: VarDeclStmt
{
  $$ = A_ProgramVarDeclStmt($1->pos, $1);
}
| StructDef
{
  $$ = A_ProgramStructDef($1->pos, $1);
}
| FnDeclStmt
{
  $$ = A_ProgramFnDeclStmt($1->pos, $1);
}
| FnDef
{
  $$ = A_ProgramFnDef($1->pos, $1);
}
| SEMICOLON
{
  $$ = A_ProgramNullStmt($1);
}
;

CodeBlockStmtList:CodeBlockStmt CodeBlockStmtList
{
  $$ = A_CodeBlockStmtList($1, $2);
}
|
{
  $$ = NULL;
}
;

CodeBlockStmt: VarDeclStmt
{
  $$ = A_BlockVarDeclStmt($1->pos, $1);
}
| AssignStmt
{
  $$ = A_BlockAssignStmt($1->pos, $1);
}
| CallStmt
{
  $$ = A_BlockCallStmt($1->pos, $1);
}
| IfStmt
{
  $$ = A_BlockIfStmt($1->pos, $1);
}
| WhileStmt
{
  $$ = A_BlockWhileStmt($1->pos, $1);
}
| ReturnStmt
{
  $$ = A_BlockReturnStmt($1->pos, $1);
}
| CONTINUE SEMICOLON
{
  $$ = A_BlockContinueStmt($1);
}
| BREAK SEMICOLON
{
  $$ = A_BlockBreakStmt($1);
}
| SEMICOLON
{
  $$ = A_BlockNullStmt($1);
}
;

AssignStmt: LeftVal ASSIGN RightVal SEMICOLON
{
  $$ = A_AssignStmt($1->pos, $1, $3);
}
;

WhileStmt: WHILE LPAR BoolExpr RPAR LBRA CodeBlockStmtList RBRA
{
  $$ = A_WhileStmt($1, $3, $6);
}
;

// ifStmt := < if > < ( > boolExpr < ) > codeBlock ( < else > codeBlock | ϵ )
IfStmt: IF LPAR BoolExpr RPAR LBRA CodeBlockStmtList RBRA ELSE LBRA CodeBlockStmtList RBRA
{
  $$ = A_IfStmt($1, $3, $6, $10);
}
| IF LPAR BoolExpr RPAR LBRA CodeBlockStmtList RBRA
{
  $$ = A_IfStmt($1, $3, $6, nullptr); // MAYBE WRONG
}
;

CallStmt: FnCall SEMICOLON
{
  $$ = A_CallStmt($1->pos, $1);
}
;

FnCall: ID LPAR RightValList RPAR
{
 $$ = A_FnCall($1->pos, $1->u.id, $3);
}
;

ReturnStmt: RET RightVal SEMICOLON
{
  $$ = A_ReturnStmt($1, $2);
}
;

VarDeclStmt: LET VarDecl SEMICOLON
{
  $$ = A_VarDeclStmt($1, $2);
}
| LET VarDef SEMICOLON
{
  $$ = A_VarDefStmt($1, $2);
}
;

ParamDecl: VarDeclList
{
  $$ = A_ParamDecl($1);
}
;

VarDeclList: VarDecl COMMA VarDeclList // MAYBE WRONG
{
  $$ = A_VarDeclList($1, $3);
}
| VarDecl
{
  $$ = A_VarDeclList($1, nullptr);
}
|
{
  $$ = NULL;
}
;

VarDecl: ID COLON Type
{
  $$ = A_VarDecl_Scalar($1->pos, A_VarDeclScalar($1->pos, $1->u.id, $3));
}
| ID LBKT NUM RBKT COLON Type
{
  $$ = A_VarDecl_Array($1->pos, A_VarDeclArray($1->pos, $1->u.id, $3->u.num, $6));
}
| ID LBKT NUM RBKT
{
  $$ = A_VarDecl_Array($1->pos, A_VarDeclArray($1->pos, $1->u.id, $3->u.num, nullptr));
}
| ID
{
  $$ = A_VarDecl_Scalar($1->pos, A_VarDeclScalar($1->pos, $1->u.id, nullptr));
}
;

VarDef: ID COLON Type ASSIGN RightVal
{
  $$ = A_VarDef_Scalar($1->pos, A_VarDefScalar($1->pos, $1->u.id, $3, $5));
}
| ID ASSIGN RightVal
{
  $$ = A_VarDef_Scalar($1->pos, A_VarDefScalar($1->pos, $1->u.id, nullptr, $3)); // primitive type? int type?
}
| ID LBKT NUM RBKT COLON Type ASSIGN LBRA RightValList RBRA 
{
  $$ = A_VarDef_Array($1->pos, A_VarDefArray($1->pos, $1->u.id, $3->u.num, $6, $9));
}
| ID LBKT NUM RBKT ASSIGN LBRA RightValList RBRA 
{
  $$ = A_VarDef_Array($1->pos, A_VarDefArray($1->pos, $1->u.id, $3->u.num, nullptr, $7));
}
;

StructDef: STRUCT ID LBRA VarDeclList RBRA // not check for empty varDeclList
{
  $$ = A_StructDef($1, $2->u.id, $4);
}
;

FnDecl: FN ID LPAR ParamDecl RPAR ARROW Type
{
  $$ = A_FnDecl($1, $2->u.id, $4, $7);
}
| FN ID LPAR ParamDecl RPAR
{
  $$ = A_FnDecl($1, $2->u.id, $4, nullptr);
}
;

FnDef: FnDecl LBRA CodeBlockStmtList RBRA
{
  $$ = A_FnDef($1->pos, $1, $3);
}
;

FnDeclStmt: FnDecl SEMICOLON
{
  $$ = A_FnDeclStmt($1->pos, $1);
}
;

Type: INT
{
  $$ = A_NativeType($1, A_intTypeKind);
}
| ID
{ 
  $$ = A_StructType($1->pos, $1->u.id);
}
;

RightValList: RightVal COMMA RightValList //MAYBE WRONG
{
  $$ = A_RightValList($1, $3);
}
| RightVal
{
  $$ = A_RightValList($1, nullptr);
}
|
{
  $$ = NULL;
}
;

RightVal: ArithExpr
{
  $$ = A_ArithExprRVal($1->pos, $1);
}
| BoolExpr
{
  $$ = A_BoolExprRVal($1->pos, $1);
}
;

LeftVal: LeftVal DOT ID
{
  $$ = A_MemberExprLVal($1->pos, A_MemberExpr($1->pos, $1, $3->u.id));
}
| LeftVal LBKT ID RBKT
{
  $$ = A_ArrExprLVal($1->pos,  A_ArrayExpr($1->pos, $1, A_IdIndexExpr($3->pos, $3->u.id)));
}
| LeftVal LBKT NUM RBKT
{
  $$ = A_ArrExprLVal($1->pos, A_ArrayExpr($1->pos, $1, A_NumIndexExpr($3->pos, $3->u.num)));
}
| ID
{
  $$ = A_IdExprLVal($1->pos, $1->u.id);
}
;

BoolExpr: BoolExpr AND BoolExpr
{
  $$ = A_BoolBiOp_Expr($1->pos, A_BoolBiOpExpr($1->pos, A_and, $1, $3));
}
| BoolExpr OR BoolExpr
{
  $$ = A_BoolBiOp_Expr($1->pos, A_BoolBiOpExpr($1->pos, A_or, $1, $3));
}
| BoolUnit
{
  $$ = A_BoolExpr($1->pos, $1);
}
;

BoolUnit: ExprUnit GREATER ExprUnit
{
  $$ = A_ComExprUnit($1->pos, A_ComExpr($1->pos, A_gt, $1, $3));
}
| ExprUnit LESS ExprUnit
{
  $$ = A_ComExprUnit($1->pos, A_ComExpr($1->pos, A_lt, $1, $3));
}
| ExprUnit UNGREATER ExprUnit
{
  $$ = A_ComExprUnit($1->pos, A_ComExpr($1->pos, A_le, $1, $3));
}
| ExprUnit UNLESS ExprUnit
{
  $$ = A_ComExprUnit($1->pos, A_ComExpr($1->pos, A_ge, $1, $3));
}
| ExprUnit EQUAL ExprUnit
{
  $$ = A_ComExprUnit($1->pos, A_ComExpr($1->pos, A_eq, $1, $3));
}
| ExprUnit UNEQUAL ExprUnit
{
  $$ = A_ComExprUnit($1->pos, A_ComExpr($1->pos, A_ne, $1, $3));
}
| LPAR BoolExpr RPAR
{
  $$ = A_BoolExprUnit($1, $2);
}
| NOT BoolUnit
{
  $$ = A_BoolUOpExprUnit($1, A_BoolUOpExpr($1, A_not, $2));
}
;

ArithExpr: ArithExpr ADD ArithExpr
{
  $$ = A_ArithBiOp_Expr($1->pos, A_ArithBiOpExpr($1->pos, A_add, $1, $3));
}
| ArithExpr SUB ArithExpr
{
  $$ = A_ArithBiOp_Expr($1->pos, A_ArithBiOpExpr($1->pos, A_sub, $1, $3));
}
| ArithExpr MUL ArithExpr
{
  $$ = A_ArithBiOp_Expr($1->pos, A_ArithBiOpExpr($1->pos, A_mul, $1, $3));
}
| ArithExpr DIV ArithExpr
{
  $$ = A_ArithBiOp_Expr($1->pos, A_ArithBiOpExpr($1->pos, A_div, $1, $3));
}
| ExprUnit
{
  $$ = A_ExprUnit($1->pos, $1);
}
;

ExprUnit: NUM
{
  $$ = A_NumExprUnit($1->pos, $1->u.num);
}
| LPAR ArithExpr RPAR
{
  $$ = A_ArithExprUnit($1, $2);
}
| FnCall
{
  $$ = A_CallExprUnit($1->pos, $1);
}
| LeftVal LBKT ID RBKT 
{
  $$ = A_ArrayExprUnit($1->pos, A_ArrayExpr($1->pos, $1, A_IdIndexExpr($3->pos, $3->u.id)));
}
| LeftVal LBKT NUM RBKT 
{
  $$ = A_ArrayExprUnit($1->pos, A_ArrayExpr($1->pos, $1, A_NumIndexExpr($3->pos, $3->u.num)));
}
| LeftVal DOT ID 
{
  $$ = A_MemberExprUnit($1->pos, A_MemberExpr($1->pos, $1, $3->u.id));
}
| SUB ExprUnit  %prec UMINUS
{
  $$ = A_ArithUExprUnit($1, A_ArithUExpr($2->pos, A_neg, $2));
}
| ID 
{
  $$ = A_IdExprUnit($1->pos, $1->u.id);
}
;

%%

extern "C"{
void yyerror(char * s)
{
  fprintf(stderr, "%s\n",s);
}
int yywrap()
{
  return(1);
}
}


