#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <time.h>
#include "TeaplAst.h"
#include "TeaplaAst.h"
#include <unordered_map>


// you can use this type to store the type a token.
typedef std::unordered_map<string, aA_type> typeMap; 
typedef std::unordered_map<string, int> intMap; 

// you can use this map to store the members of a struct or params of a function.
typedef std::unordered_map<string, vector<aA_varDecl>*> paramMemberMap;


void check_Prog(std::ostream* out, aA_program p);
void check_VarDecl(std::ostream* out, aA_varDeclStmt vd);
void check_StructDef(std::ostream* out, aA_structDef sd);
void check_FnDecl(std::ostream* out, aA_fnDecl fd);
void check_FnDeclStmt(std::ostream* out, aA_fnDeclStmt fd);
void check_FnDef(std::ostream* out, aA_fnDef fd);
void check_AssignStmt(std::ostream* out, aA_assignStmt as);
aA_type check_ArrayExpr(std::ostream* out, aA_arrayExpr ae);
aA_type check_MemberExpr(std::ostream* out, aA_memberExpr me);
void check_IfStmt(std::ostream* out, aA_ifStmt is);
void check_BoolExpr(std::ostream* out, aA_boolExpr be);
void check_BoolUnit(std::ostream* out, aA_boolUnit bu);
aA_type check_ExprUnit(std::ostream* out, aA_exprUnit eu);
aA_type check_FuncCall(std::ostream* out, aA_fnCall fc);
void check_WhileStmt(std::ostream* out, aA_whileStmt ws);
void check_CallStmt(std::ostream* out, aA_callStmt cs);
void check_ReturnStmt(std::ostream* out, aA_returnStmt rs);

void check_CodeblockStmt(std::ostream* out, aA_codeBlockStmt cs, vector<string>* sub_token);
void check_CodeblockStmtList(std::ostream* out, vector<aA_codeBlockStmt> blocks);
void check_ArithBiOpExpr(std::ostream* out, aA_arithBiOpExpr ae);
void struct_type_check(std::ostream* out, A_pos pos, aA_type type);
void assign_type_check(std::ostream* out, aA_type type, aA_rightVal val);
string get_type(aA_type type);
aA_type check_VarExpr(std::ostream* out, A_pos pos, string name);
void add_VarDecl(aA_varDeclStmt vd, bool global, vector<string>* s);