#include "ast2llvm.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <cassert>
#include <list>
#include<iostream>
using namespace std;
using namespace LLVMIR;

static unordered_map<string,FuncType> funcReturnMap;
static unordered_map<string,StructInfo> structInfoMap;
static unordered_map<string,Name_name*> globalVarMap;
static unordered_map<string,Temp_temp*> localVarMap;
static list<L_stm*> emit_irs;

bool blockEnd = false; // meet break continue and return

LLVMIR::L_prog* ast2llvm(aA_program p)
{
    auto defs = ast2llvmProg_first(p);
    auto funcs = ast2llvmProg_second(p);
    vector<L_func*> funcs_block;
    for(const auto &f : funcs)
    {
        funcs_block.push_back(ast2llvmFuncBlock(f));
    }
    for(auto &f : funcs_block)
    {
        ast2llvm_moveAlloca(f);
    }
    return new L_prog(defs,funcs_block);
}

int ast2llvmRightVal_first(aA_rightVal r)
{
    if(r == nullptr)
    {
        return 0;
    }
    switch (r->kind)
    {
    case A_arithExprValKind:
    {
        return ast2llvmArithExpr_first(r->u.arithExpr);
        break;
    }
    case A_boolExprValKind:
    {
        return ast2llvmBoolExpr_first(r->u.boolExpr);
        break;
    }
    default:
        break;
    }
    return 0;
}

int ast2llvmBoolExpr_first(aA_boolExpr b)
{
    switch (b->kind)
    {
    case A_boolBiOpExprKind:
    {
        return ast2llvmBoolBiOpExpr_first(b->u.boolBiOpExpr);
        break;
    }
    case A_boolUnitKind:
    {
        return ast2llvmBoolUnit_first(b->u.boolUnit);
        break;
    }
    default:
         break;
    }
    return 0;
}

int ast2llvmBoolBiOpExpr_first(aA_boolBiOpExpr b)
{
    int l = ast2llvmBoolExpr_first(b->left);
    int r = ast2llvmBoolExpr_first(b->right);
    if(b->op == A_and)
    {
        return l && r;
    }
    else
    {
        return l || r;
    }
}

int ast2llvmBoolUOpExpr_first(aA_boolUOpExpr b)
{
    if(b->op == A_not)
    {
        return !ast2llvmBoolUnit_first(b->cond);
    }
    return 0;
}

int ast2llvmBoolUnit_first(aA_boolUnit b)
{
    switch (b->kind)
    {
    case A_comOpExprKind:
    {
        return ast2llvmComOpExpr_first(b->u.comExpr);
        break;
    }
    case A_boolExprKind:
    {
        return ast2llvmBoolExpr_first(b->u.boolExpr);
        break;
    }
    case A_boolUOpExprKind:
    {
        return ast2llvmBoolUOpExpr_first(b->u.boolUOpExpr);
        break;
    }
    default:
        break;
    }
    return 0;
}

int ast2llvmComOpExpr_first(aA_comExpr c)
{
    auto l = ast2llvmExprUnit_first(c->left);
    auto r = ast2llvmExprUnit_first(c->right);
    switch (c->op)
    {
    case A_lt:
    {
        return l < r;
        break;
    }
    case A_le:
    {
        return l <= r;
        break;
    }
    case A_gt:
    {
        return l > r;
        break;
    }
    case A_ge:
    {
        return l >= r;
        break;
    }
    case A_eq:
    {
        return l == r;
        break;
    }
    case A_ne:
    {
        return l != r;
        break;
    }
    default:
        break;
    }
    return 0;
}

int ast2llvmArithBiOpExpr_first(aA_arithBiOpExpr a)
{
    auto l= ast2llvmArithExpr_first(a->left);
    auto r = ast2llvmArithExpr_first(a->right);
    switch (a->op)
    {
    case A_add:
    {
        return l + r;
        break;
    }
    case A_sub:
    {
        return l - r;
        break;
    }
    case A_mul:
    {
        return l * r;
        break;
    }
    case A_div:
    {
        return l / r;
        break;
    }
    default:
        break;
    }
    return 0;
}

int ast2llvmArithUExpr_first(aA_arithUExpr a)
{
    if(a->op == A_neg)
    {
        return -ast2llvmExprUnit_first(a->expr);
    }
    return 0;
}

int ast2llvmArithExpr_first(aA_arithExpr a)
{
    switch (a->kind)
    {
    case A_arithBiOpExprKind:
    {
        return ast2llvmArithBiOpExpr_first(a->u.arithBiOpExpr);
        break;
    }
    case A_exprUnitKind:
    {
        return ast2llvmExprUnit_first(a->u.exprUnit);
        break;
    }
    default:
        assert(0);
        break;
    }
    return 0;
}

int ast2llvmExprUnit_first(aA_exprUnit e)
{
    if(e->kind == A_numExprKind)
    {
        return e->u.num;
    }
    else if(e->kind == A_arithExprKind)
    {
        return ast2llvmArithExpr_first(e->u.arithExpr);
    }
    else if(e->kind == A_arithUExprKind)
    {
        return ast2llvmArithUExpr_first(e->u.arithUExpr);
    }
    else
    {
        assert(0);
    }
    return 0;
}

std::vector<LLVMIR::L_def*> ast2llvmProg_first(aA_program p)
{
    vector<L_def*> defs;
    defs.push_back(L_Funcdecl("getch",vector<TempDef>(),FuncType(ReturnType::INT_TYPE)));
    defs.push_back(L_Funcdecl("getint",vector<TempDef>(),FuncType(ReturnType::INT_TYPE)));
    defs.push_back(L_Funcdecl("putch",vector<TempDef>{TempDef(TempType::INT_TEMP)},FuncType(ReturnType::VOID_TYPE)));
    defs.push_back(L_Funcdecl("putint",vector<TempDef>{TempDef(TempType::INT_TEMP)},FuncType(ReturnType::VOID_TYPE)));
    defs.push_back(L_Funcdecl("putarray",vector<TempDef>{TempDef(TempType::INT_TEMP),TempDef(TempType::INT_PTR,-1)},FuncType(ReturnType::VOID_TYPE)));
    defs.push_back(L_Funcdecl("_sysy_starttime",vector<TempDef>{TempDef(TempType::INT_TEMP)},FuncType(ReturnType::VOID_TYPE)));
    defs.push_back(L_Funcdecl("_sysy_stoptime",vector<TempDef>{TempDef(TempType::INT_TEMP)},FuncType(ReturnType::VOID_TYPE)));
    funcReturnMap.emplace("getch",FuncType(ReturnType::INT_TYPE));
    funcReturnMap.emplace("getint",FuncType(ReturnType::INT_TYPE));
    funcReturnMap.emplace("putch",FuncType(ReturnType::VOID_TYPE));
    funcReturnMap.emplace("putint",FuncType(ReturnType::VOID_TYPE));
    funcReturnMap.emplace("putarray",FuncType(ReturnType::VOID_TYPE));
    funcReturnMap.emplace("_sysy_starttime",FuncType(ReturnType::VOID_TYPE));
    funcReturnMap.emplace("_sysy_stoptime",FuncType(ReturnType::VOID_TYPE));
    for(const auto &v : p->programElements)
    {
        switch (v->kind)
        {
        case A_programNullStmtKind:
        {
            break;
        }
        case A_programVarDeclStmtKind:
        {
            if(v->u.varDeclStmt->kind == A_varDeclKind)
            {
                if(v->u.varDeclStmt->u.varDecl->kind == A_varDeclScalarKind)
                {
                    if(v->u.varDeclStmt->u.varDecl->u.declScalar->type->type == A_structTypeKind)
                    {
                        globalVarMap.emplace(*v->u.varDeclStmt->u.varDecl->u.declScalar->id,
                            Name_newname_struct(Temp_newlabel_named(*v->u.varDeclStmt->u.varDecl->u.declScalar->id),*v->u.varDeclStmt->u.varDecl->u.declScalar->type->u.structType));
                        TempDef def(TempType::STRUCT_TEMP,0,*v->u.varDeclStmt->u.varDecl->u.declScalar->type->u.structType);
                        defs.push_back(L_Globaldef(*v->u.varDeclStmt->u.varDecl->u.declScalar->id,def,vector<int>()));
                    }
                    else
                    {
                        globalVarMap.emplace(*v->u.varDeclStmt->u.varDecl->u.declScalar->id,
                            Name_newname_int(Temp_newlabel_named(*v->u.varDeclStmt->u.varDecl->u.declScalar->id)));
                        TempDef def(TempType::INT_TEMP,0);
                        defs.push_back(L_Globaldef(*v->u.varDeclStmt->u.varDecl->u.declScalar->id,def,vector<int>()));
                    }
                }
                else if(v->u.varDeclStmt->u.varDecl->kind == A_varDeclArrayKind)
                {
                    if(v->u.varDeclStmt->u.varDecl->u.declArray->type->type == A_structTypeKind)
                    {
                        globalVarMap.emplace(*v->u.varDeclStmt->u.varDecl->u.declArray->id,
                            Name_newname_struct_ptr(Temp_newlabel_named(*v->u.varDeclStmt->u.varDecl->u.declArray->id),v->u.varDeclStmt->u.varDecl->u.declArray->len,*v->u.varDeclStmt->u.varDecl->u.declArray->type->u.structType));
                        TempDef def(TempType::STRUCT_PTR,v->u.varDeclStmt->u.varDecl->u.declArray->len,*v->u.varDeclStmt->u.varDecl->u.declArray->type->u.structType);
                        defs.push_back(L_Globaldef(*v->u.varDeclStmt->u.varDecl->u.declArray->id,def,vector<int>()));
                    }
                    else
                    {
                        globalVarMap.emplace(*v->u.varDeclStmt->u.varDecl->u.declArray->id,
                            Name_newname_int_ptr(Temp_newlabel_named(*v->u.varDeclStmt->u.varDecl->u.declArray->id),v->u.varDeclStmt->u.varDecl->u.declArray->len));
                        TempDef def(TempType::INT_PTR,v->u.varDeclStmt->u.varDecl->u.declArray->len);
                        defs.push_back(L_Globaldef(*v->u.varDeclStmt->u.varDecl->u.declArray->id,def,vector<int>()));
                    }
                }
                else
                {
                    assert(0);
                }
            }
            else if(v->u.varDeclStmt->kind == A_varDefKind)
            {
                if(v->u.varDeclStmt->u.varDef->kind == A_varDefScalarKind)
                {
                    if(v->u.varDeclStmt->u.varDef->u.defScalar->type->type == A_structTypeKind)
                    {
                        globalVarMap.emplace(*v->u.varDeclStmt->u.varDef->u.defScalar->id,
                            Name_newname_struct(Temp_newlabel_named(*v->u.varDeclStmt->u.varDef->u.defScalar->id),*v->u.varDeclStmt->u.varDef->u.defScalar->type->u.structType));
                        TempDef def(TempType::STRUCT_TEMP,0,*v->u.varDeclStmt->u.varDef->u.defScalar->type->u.structType);
                        defs.push_back(L_Globaldef(*v->u.varDeclStmt->u.varDef->u.defScalar->id,def,vector<int>()));
                    }
                    else
                    {
                        globalVarMap.emplace(*v->u.varDeclStmt->u.varDef->u.defScalar->id,
                            Name_newname_int(Temp_newlabel_named(*v->u.varDeclStmt->u.varDef->u.defScalar->id)));
                        TempDef def(TempType::INT_TEMP,0);
                        vector<int> init;
                        init.push_back(ast2llvmRightVal_first(v->u.varDeclStmt->u.varDef->u.defScalar->val));
                        defs.push_back(L_Globaldef(*v->u.varDeclStmt->u.varDef->u.defScalar->id,def,init));
                    }
                }
                else if(v->u.varDeclStmt->u.varDef->kind == A_varDefArrayKind)
                {
                    if(v->u.varDeclStmt->u.varDef->u.defArray->type->type == A_structTypeKind)
                    {
                        globalVarMap.emplace(*v->u.varDeclStmt->u.varDef->u.defArray->id,
                            Name_newname_struct_ptr(Temp_newlabel_named(*v->u.varDeclStmt->u.varDef->u.defArray->id),v->u.varDeclStmt->u.varDef->u.defArray->len,*v->u.varDeclStmt->u.varDef->u.defArray->type->u.structType));
                        TempDef def(TempType::STRUCT_PTR,v->u.varDeclStmt->u.varDef->u.defArray->len,*v->u.varDeclStmt->u.varDef->u.defArray->type->u.structType);
                        defs.push_back(L_Globaldef(*v->u.varDeclStmt->u.varDef->u.defArray->id,def,vector<int>()));
                    }
                    else
                    {
                        globalVarMap.emplace(*v->u.varDeclStmt->u.varDef->u.defArray->id,
                            Name_newname_int_ptr(Temp_newlabel_named(*v->u.varDeclStmt->u.varDef->u.defArray->id),v->u.varDeclStmt->u.varDef->u.defArray->len));
                        TempDef def(TempType::INT_PTR,v->u.varDeclStmt->u.varDef->u.defArray->len);
                        vector<int> init;
                        for(auto &el : v->u.varDeclStmt->u.varDef->u.defArray->vals)
                        {
                            init.push_back(ast2llvmRightVal_first(el));
                        }
                        defs.push_back(L_Globaldef(*v->u.varDeclStmt->u.varDef->u.defArray->id,def,init));
                    }
                }
                else
                {
                    assert(0);
                }
            }
            else
            {
                assert(0);
            }
            break;
        }
        case A_programStructDefKind:
        {
            StructInfo si;
            int off = 0;
            vector<TempDef> members;
            for(const auto &decl : v->u.structDef->varDecls)
            {
                if(decl->kind == A_varDeclScalarKind)
                {
                    if(decl->u.declScalar->type->type == A_structTypeKind)
                    {
                        TempDef def(TempType::STRUCT_TEMP,0,*decl->u.declScalar->type->u.structType);
                        MemberInfo info(off++,def);
                        si.memberinfos.emplace(*decl->u.declScalar->id,info);
                        members.push_back(def);
                    }
                    else
                    {
                        TempDef def(TempType::INT_TEMP,0);
                        MemberInfo info(off++,def);
                        si.memberinfos.emplace(*decl->u.declScalar->id,info);
                        members.push_back(def);
                    }
                }
                else if(decl->kind == A_varDeclArrayKind)
                {
                    if(decl->u.declArray->type->type == A_structTypeKind)
                    {
                        TempDef def(TempType::STRUCT_PTR,decl->u.declArray->len,*decl->u.declArray->type->u.structType);
                        MemberInfo info(off++,def);
                        si.memberinfos.emplace(*decl->u.declArray->id,info);
                        members.push_back(def);
                    }
                    else
                    {
                        TempDef def(TempType::INT_PTR,decl->u.declArray->len);
                        MemberInfo info(off++,def);
                        si.memberinfos.emplace(*decl->u.declArray->id,info);
                        members.push_back(def);
                    }
                }
                else
                {
                    assert(0);
                }
            }
            structInfoMap.emplace(*v->u.structDef->id,std::move(si));
            defs.push_back(L_Structdef(*v->u.structDef->id,members));
            break;
        }
        case A_programFnDeclStmtKind:
        {
            FuncType type;
            if(v->u.fnDeclStmt->fnDecl->type == nullptr)
            {
                type.type = ReturnType::VOID_TYPE;
            }
            if(v->u.fnDeclStmt->fnDecl->type->type == A_nativeTypeKind)
            {
                type.type = ReturnType::INT_TYPE;
            }
            else if(v->u.fnDeclStmt->fnDecl->type->type == A_structTypeKind)
            {
                type.type = ReturnType::STRUCT_TYPE;
                type.structname = *v->u.fnDeclStmt->fnDecl->type->u.structType;
            }
            else
            {
                assert(0);
            }
            if(funcReturnMap.find(*v->u.fnDeclStmt->fnDecl->id) == funcReturnMap.end())
                funcReturnMap.emplace(*v->u.fnDeclStmt->fnDecl->id,std::move(type));
            vector<TempDef> args;
            for(const auto & decl : v->u.fnDeclStmt->fnDecl->paramDecl->varDecls)
            {
                if(decl->kind == A_varDeclScalarKind)
                {
                    if(decl->u.declScalar->type->type == A_structTypeKind)
                    {
                        TempDef def(TempType::STRUCT_PTR,0,*decl->u.declScalar->type->u.structType);
                        args.push_back(def);
                    }
                    else
                    {
                        TempDef def(TempType::INT_TEMP,0);
                        args.push_back(def);
                    }
                }
                else if(decl->kind == A_varDeclArrayKind)
                {
                    if(decl->u.declArray->type->type == A_structTypeKind)
                    {
                        TempDef def(TempType::STRUCT_PTR,-1,*decl->u.declArray->type->u.structType);
                        args.push_back(def);
                    }
                    else
                    {
                        TempDef def(TempType::INT_PTR,-1);
                        args.push_back(def);
                    }
                }
                else
                {
                    assert(0);
                }
            }
            defs.push_back(L_Funcdecl(*v->u.fnDeclStmt->fnDecl->id,args,type));
            break;
        }
        case A_programFnDefKind:
        {
            if(funcReturnMap.find(*v->u.fnDef->fnDecl->id) == funcReturnMap.end())
            {
                FuncType type;
                if(v->u.fnDef->fnDecl->type == nullptr)
                {
                    type.type = ReturnType::VOID_TYPE;
                }
                else if(v->u.fnDef->fnDecl->type->type == A_nativeTypeKind)
                {
                    type.type = ReturnType::INT_TYPE;
                }
                else if(v->u.fnDef->fnDecl->type->type == A_structTypeKind)
                {
                    type.type = ReturnType::STRUCT_TYPE;
                    type.structname = *v->u.fnDef->fnDecl->type->u.structType;
                }
                else
                {
                    assert(0);
                }
                funcReturnMap.emplace(*v->u.fnDef->fnDecl->id,std::move(type));
            }
            break;
        }
        default:
            assert(0);
            break;
        }
    }
    return defs;
}

std::vector<Func_local*> ast2llvmProg_second(aA_program p)
{
    vector<Func_local*> funcs;
    for(const auto &v : p->programElements)
    {
        switch (v->kind)
        {
        case A_programNullStmtKind:
            break;
        case A_programVarDeclStmtKind:
            break;
        case A_programStructDefKind:
            break;
        case A_programFnDeclStmtKind:
            break;
        case A_programFnDefKind:
        {
            funcs.push_back(ast2llvmFunc(v->u.fnDef));
        }
            break;
        }

    }
    
    return funcs;
}

Func_local* ast2llvmFunc(aA_fnDef f)
{
    emit_irs.clear();
    localVarMap.clear();

    string name = *f->fnDecl->id;
    FuncType type = f->fnDecl->type == nullptr ? FuncType(ReturnType::VOID_TYPE) : funcReturnMap[*f->fnDecl->id];

    Temp_label *start = Temp_newlabel();
    emit_irs.push_back(L_Label(start));

    vector<Temp_temp*> args;
    list<Temp_temp*> int_args;
    for(const auto & decl : f->fnDecl->paramDecl->varDecls)
    {
        // add to localmap
        if(decl->kind == A_varDeclScalarKind)
        {
            if(decl->u.declScalar->type->type == A_structTypeKind)
            {
                Temp_temp *temp = Temp_newtemp_struct_ptr(0, *decl->u.declScalar->type->u.structType);
                localVarMap.emplace(*decl->u.declScalar->id, temp);
                args.push_back(temp);
            }
            else
            {
                // add store!
                Temp_temp *temp = Temp_newtemp_int();
                args.push_back(temp);
                int_args.push_back(temp);
            }
        }
        else if(decl->kind == A_varDeclArrayKind)
        {
            if(decl->u.declArray->type->type == A_structTypeKind)
            {
                Temp_temp *temp = Temp_newtemp_struct_ptr(-1, *decl->u.declArray->type->u.structType);
                localVarMap.emplace(*decl->u.declArray->id, temp);
                args.push_back(temp);
            }
            else
            {
                Temp_temp *temp = Temp_newtemp_int_ptr(-1);
                localVarMap.emplace(*decl->u.declArray->id, temp);
                args.push_back(temp);
            }
        }
        else
        {
            assert(0);
        }
    }
    // add load for int vars
    for(const auto & decl : f->fnDecl->paramDecl->varDecls)
    {
        if(decl->kind == A_varDeclScalarKind)
        {
            if(decl->u.declScalar->type->type == A_nativeTypeKind)
            {        
                Temp_temp *var = Temp_newtemp_int_ptr(0);
                localVarMap.emplace(*decl->u.declScalar->id, var);
                L_stm *stm = L_Alloca(AS_Operand_Temp(var));
                emit_irs.push_back(stm);

                L_stm *stm2 = L_Store(AS_Operand_Temp(int_args.front()), AS_Operand_Temp(var));
                emit_irs.push_back(stm2);
                
                int_args.pop_front();
            }

        }
    }
    for(Temp_temp* arg : int_args) {

    }
    // list<LLVMIR::L_stm*> irs;

    vector<aA_codeBlockStmt> blocks = f->stmts;
    for(aA_codeBlockStmt block : blocks)
    {
        ast2llvmBlock(block);
        if(blockEnd) {
            blockEnd = false;
            break;
        }
    }
    if(emit_irs.back()->type != L_StmKind::T_RETURN) {
        if(type.type == ReturnType::VOID_TYPE) 
            emit_irs.push_back(L_Ret(nullptr));
        else
            emit_irs.push_back(L_Ret(AS_Operand_Const(0)));
    }
    return new Func_local(name,type,args,emit_irs);
}

void ast2llvmBlock(aA_codeBlockStmt b,Temp_label *con_label,Temp_label *bre_label)
{
    aA_codeBlockStmt block = b;
    switch (block->kind)
    {
    case A_codeBlockStmtType::A_varDeclStmtKind:
    {
        if(block->u.varDeclStmt->kind == A_varDeclKind)
        {
            if(block->u.varDeclStmt->u.varDecl->kind == A_varDeclScalarKind)
            {
                if(block->u.varDeclStmt->u.varDecl->u.declScalar->type->type == A_structTypeKind)
                {
                    if(localVarMap.find(*block->u.varDeclStmt->u.varDecl->u.declScalar->id) != localVarMap.end()) { // declared in previous scope
                        localVarMap.erase(*block->u.varDeclStmt->u.varDecl->u.declScalar->id);
                    }
                    localVarMap.emplace(*block->u.varDeclStmt->u.varDecl->u.declScalar->id,
                        Temp_newtemp_struct_ptr(0, *block->u.varDeclStmt->u.varDecl->u.declScalar->type->u.structType));
                    L_stm *stm = L_Alloca(AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDecl->u.declScalar->id]));
                    emit_irs.push_back(stm);
                    
            
                }
                else
                {
                    if(localVarMap.find(*block->u.varDeclStmt->u.varDecl->u.declScalar->id) != localVarMap.end()) { // declared in previous scope
                        localVarMap.erase(*block->u.varDeclStmt->u.varDecl->u.declScalar->id);
                    }
                    if(localVarMap.find(*block->u.varDeclStmt->u.varDecl->u.declScalar->id) == localVarMap.end())
                    localVarMap.emplace(*block->u.varDeclStmt->u.varDecl->u.declScalar->id,
                        Temp_newtemp_int_ptr(0));
                    L_stm *stm = L_Alloca(AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDecl->u.declScalar->id]));
                    emit_irs.push_back(stm);
                }
            }
            else if(block->u.varDeclStmt->u.varDecl->kind == A_varDeclArrayKind)
            {
                if(block->u.varDeclStmt->u.varDecl->u.declArray->type->type == A_structTypeKind)
                {
                    if(localVarMap.find(*block->u.varDeclStmt->u.varDecl->u.declArray->id) != localVarMap.end()) { // declared in previous scope
                        localVarMap.erase(*block->u.varDeclStmt->u.varDecl->u.declArray->id);
                    }
                    localVarMap.emplace(*block->u.varDeclStmt->u.varDecl->u.declArray->id,
                        Temp_newtemp_struct_ptr(block->u.varDeclStmt->u.varDecl->u.declArray->len, *block->u.varDeclStmt->u.varDecl->u.declArray->type->u.structType));
                    L_stm *stm = L_Alloca(AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDecl->u.declArray->id]));
                    emit_irs.push_back(stm); 
                }
                else
                {
                    if(localVarMap.find(*block->u.varDeclStmt->u.varDecl->u.declArray->id) != localVarMap.end()) { // declared in previous scope
                        localVarMap.erase(*block->u.varDeclStmt->u.varDecl->u.declArray->id);
                    }
                    localVarMap.emplace(*block->u.varDeclStmt->u.varDecl->u.declArray->id,
                        Temp_newtemp_int_ptr(block->u.varDeclStmt->u.varDecl->u.declArray->len));
                    L_stm *stm = L_Alloca(AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDecl->u.declArray->id]));
                    emit_irs.push_back(stm);
                }
            }
            else
            {
                assert(0);
            }
        }
        else if(block->u.varDeclStmt->kind == A_varDefKind)
        {
            if(block->u.varDeclStmt->u.varDef->kind == A_varDefScalarKind)
            {
                if(block->u.varDeclStmt->u.varDef->u.defScalar->type->type == A_structTypeKind)
                {
                    if(localVarMap.find(*block->u.varDeclStmt->u.varDef->u.defScalar->id) != localVarMap.end()) { // declared in previous scope
                        localVarMap.erase(*block->u.varDeclStmt->u.varDef->u.defScalar->id);
                    }
                    localVarMap.emplace(*block->u.varDeclStmt->u.varDef->u.defScalar->id,
                        Temp_newtemp_struct_ptr(0, *block->u.varDeclStmt->u.varDef->u.defScalar->type->u.structType));
                    L_stm *stm = L_Alloca(AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDef->u.defScalar->id]));
                    emit_irs.push_back(stm);

                    AS_operand* right = ast2llvmRightVal(block->u.varDeclStmt->u.varDef->u.defScalar->val);

                    L_stm *stm2 = L_Store(right, AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDef->u.defScalar->id]));
                    emit_irs.push_back(stm2);
                }
                else
                {
                    if(localVarMap.find(*block->u.varDeclStmt->u.varDef->u.defScalar->id) != localVarMap.end()) { // declared in previous scope
                        localVarMap.erase(*block->u.varDeclStmt->u.varDef->u.defScalar->id);
                    }
                    localVarMap.emplace(*block->u.varDeclStmt->u.varDef->u.defScalar->id,
                        Temp_newtemp_int_ptr(0));
                    L_stm *stm = L_Alloca(AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDef->u.defScalar->id]));
                    emit_irs.push_back(stm);

                    AS_operand* right = ast2llvmRightVal(block->u.varDeclStmt->u.varDef->u.defScalar->val);

                    L_stm *stm2 = L_Store(right, AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDef->u.defScalar->id]));
                    emit_irs.push_back(stm2);
                }
            }
            else if(block->u.varDeclStmt->u.varDef->kind == A_varDefArrayKind)
            {
                if(block->u.varDeclStmt->u.varDef->u.defArray->type->type == A_structTypeKind)
                {
                    if(localVarMap.find(*block->u.varDeclStmt->u.varDef->u.defArray->id) != localVarMap.end()) { // declared in previous scope
                        localVarMap.erase(*block->u.varDeclStmt->u.varDef->u.defArray->id);
                    }
                    localVarMap.emplace(*block->u.varDeclStmt->u.varDef->u.defArray->id,
                        Temp_newtemp_struct_ptr(block->u.varDeclStmt->u.varDef->u.defArray->len, *block->u.varDeclStmt->u.varDecl->u.declArray->type->u.structType));
                    L_stm *stm = L_Alloca(AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDef->u.defArray->id]));
                    emit_irs.push_back(stm); 

                    for(int i = 0; i < block->u.varDeclStmt->u.varDef->u.defArray->vals.size(); i++) {
                        aA_rightVal val = block->u.varDeclStmt->u.varDef->u.defArray->vals[i];
                        AS_operand* right = ast2llvmRightVal(val);
                        // AS_operand* index = AS_Operand_Const(i);
                        //here strange
                        AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_struct_ptr(0, *block->u.varDeclStmt->u.varDecl->u.declArray->type->u.structType));
                        L_stm *stm2 = L_Gep(tmp, 
                            AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDef->u.defArray->id]),
                            AS_Operand_Const(i));
                        emit_irs.push_back(stm2);

                        L_stm *stm3 = L_Store(right, tmp);
                        emit_irs.push_back(stm3);
                    }
                }
                else
                {
                    if(localVarMap.find(*block->u.varDeclStmt->u.varDef->u.defArray->id) != localVarMap.end()) { // declared in previous scope
                        localVarMap.erase(*block->u.varDeclStmt->u.varDef->u.defArray->id);
                    }
                    localVarMap.emplace(*block->u.varDeclStmt->u.varDef->u.defArray->id,
                        Temp_newtemp_int_ptr(block->u.varDeclStmt->u.varDef->u.defArray->len));
                    L_stm *stm = L_Alloca(AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDef->u.defArray->id]));
                    emit_irs.push_back(stm);

                    for(int i = 0; i < block->u.varDeclStmt->u.varDef->u.defArray->vals.size(); i++) {
                        aA_rightVal val = block->u.varDeclStmt->u.varDef->u.defArray->vals[i];
                        AS_operand* right = ast2llvmRightVal(val);
                        // AS_operand* index = AS_Operand_Const(i);
                        //here strange
                        AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_int_ptr(0));
                        L_stm *stm2 = L_Gep(tmp, 
                            AS_Operand_Temp(localVarMap[*block->u.varDeclStmt->u.varDef->u.defArray->id]),
                            AS_Operand_Const(i));
                        emit_irs.push_back(stm2);

                        L_stm *stm3 = L_Store(right, tmp);
                        emit_irs.push_back(stm3);
                    }
                }
            }
            else
            {
                assert(0);
            }
        }
        else
        {
            assert(0);
        }
    }
        break;
    case A_codeBlockStmtType::A_assignStmtKind:
    {
        AS_operand* right = ast2llvmRightVal(block->u.assignStmt->rightVal);
        AS_operand* left = ast2llvmLeftVal(block->u.assignStmt->leftVal);
        L_stm *stm = L_Store(right, left);
        emit_irs.push_back(stm);
    }
        break;
    case A_codeBlockStmtType::A_ifStmtKind:
    {   
        if(!block->u.ifStmt->elseStmts.empty())
        {
            Temp_label *true_label = Temp_newlabel();
            Temp_label *false_label = Temp_newlabel();
            Temp_label *end_label = Temp_newlabel();
            AS_operand *cond = ast2llvmBoolExpr(block->u.ifStmt->boolExpr, true_label, false_label);
            // L_stm *stm = L_Cjump(cond, true_label, false_label);
            // emit_irs.push_back(stm);
            L_stm *stm2 = L_Label(true_label);
            emit_irs.push_back(stm2);
            for(aA_codeBlockStmt block : block->u.ifStmt->ifStmts)
            {
                ast2llvmBlock(block, con_label, bre_label);
                if(blockEnd) {
                    break;
                }
            }
            if(!blockEnd) {
                L_stm *stm3 = L_Jump(end_label);
                emit_irs.push_back(stm3);
            }
            blockEnd = false;

            L_stm *stm4 = L_Label(false_label);
            emit_irs.push_back(stm4);
            for(aA_codeBlockStmt block : block->u.ifStmt->elseStmts)
            {
                ast2llvmBlock(block, con_label, bre_label);
                if(blockEnd) {
                    break;
                }
            }
            if(!blockEnd) {
                L_stm *stm5 = L_Jump(end_label);
                emit_irs.push_back(stm5);
            }
            blockEnd = false;

            L_stm *stm6 = L_Label(end_label);
            emit_irs.push_back(stm6);
        }
        else {
            Temp_label *true_label = Temp_newlabel();
            Temp_label *end_label = Temp_newlabel();
            AS_operand *cond = ast2llvmBoolExpr(block->u.ifStmt->boolExpr, true_label, end_label);
            // L_stm *stm = L_Cjump(cond, true_label, end_label);
            // emit_irs.push_back(stm);
           
            L_stm *stm2 = L_Label(true_label);
            emit_irs.push_back(stm2);
            for(aA_codeBlockStmt block : block->u.ifStmt->ifStmts)
            {
                ast2llvmBlock(block,con_label,bre_label);
                if(blockEnd) {
                    break;
                }
            }
            if(!blockEnd) {
                L_stm *stm3 = L_Jump(end_label);
                emit_irs.push_back(stm3);
            }
            blockEnd = false;
 
            L_stm *stm4 = L_Label(end_label);
            emit_irs.push_back(stm4);
        }
    }            
        break;
    case A_codeBlockStmtType::A_whileStmtKind:
    {
        Temp_label *con_label = Temp_newlabel();
        Temp_label *block_label = Temp_newlabel();
        Temp_label *bre_label = Temp_newlabel();
        L_stm *stm0 =L_Jump(con_label);
        emit_irs.push_back(stm0);
        L_stm *stm = L_Label(con_label);
        emit_irs.push_back(stm);
        AS_operand *cond = ast2llvmBoolExpr(block->u.whileStmt->boolExpr, block_label, bre_label);
        // L_stm *stm2 = L_Cjump(cond, block_label, bre_label);
        // emit_irs.push_back(stm2);
        L_stm *stm3 = L_Label(block_label);
        emit_irs.push_back(stm3);
        for(aA_codeBlockStmt block : block->u.whileStmt->whileStmts)
        {
            ast2llvmBlock(block,con_label,bre_label);
            if(blockEnd) {
                break;
            }
        }
        blockEnd = false;
        L_stm *stm4 = L_Jump(con_label);
        emit_irs.push_back(stm4);
        L_stm *stm5 = L_Label(bre_label);
        emit_irs.push_back(stm5);
    }
        break;
    case A_codeBlockStmtType::A_callStmtKind:
    {
        vector<AS_operand*> args;
        for(const aA_rightVal arg : block->u.callStmt->fnCall->vals)
        {
            args.push_back(ast2llvmRightVal(arg));
        }
        // funcReturnMap.emplace(*v->u.fnDef->fnDecl->id,std::move(type));
        // FuncType type = funcReturnMap[*block->u.callStmt->fnCall->fn];
        // ReturnType ret = type.type;
        if(funcReturnMap[*block->u.callStmt->fnCall->fn].type == ReturnType::VOID_TYPE)
        {
            L_stm *stm = L_Voidcall(*block->u.callStmt->fnCall->fn, args);
            emit_irs.push_back(stm);
        } 
        else 
        {
            AS_operand* ret = AS_Operand_Temp(Temp_newtemp_int()); // ret is not used
            L_stm *stm = L_Call(*block->u.callStmt->fnCall->fn, ret, args);
            emit_irs.push_back(stm);
        }
    }
        break;
    case A_codeBlockStmtType::A_returnStmtKind:
    {
        if(block->u.returnStmt->retVal == nullptr)
        {
            L_stm *stm = L_Ret(nullptr);
            emit_irs.push_back(stm);
        }
        else
        {
            L_stm *stm = L_Ret(ast2llvmRightVal(block->u.returnStmt->retVal));
            emit_irs.push_back(stm);
        }
        blockEnd = true;
    }
        break;
    case A_codeBlockStmtType::A_breakStmtKind:
    {
        L_stm *stm = L_Jump(bre_label);
        emit_irs.push_back(stm);
        blockEnd = true;
    }
        break;
    case A_codeBlockStmtType::A_continueStmtKind:
    {
        L_stm *stm = L_Jump(con_label);
        emit_irs.push_back(stm);
        blockEnd = true;
    }
        break;
    default:
        break;
    }
}

AS_operand* ast2llvmRightVal(aA_rightVal r)
{
    switch (r->kind)
    {
    case A_rightValType::A_arithExprValKind:
    {
        return ast2llvmArithExpr(r->u.arithExpr);
    }
        break;
    case A_rightValType::A_boolExprValKind:
    {
        Temp_label *true_label = Temp_newlabel();
        Temp_label *false_label = Temp_newlabel();
        Temp_label *end_label = Temp_newlabel();

        AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_int_ptr(0));
        L_stm *stm0 = L_Alloca(tmp);
        emit_irs.push_back(stm0);
        ast2llvmBoolExpr(r->u.boolExpr, true_label, false_label);
    
        L_stm *stm = L_Label(true_label);
        emit_irs.push_back(stm);
        L_stm *stm2 = L_Store(AS_Operand_Const(1), tmp);
        emit_irs.push_back(stm2);
        L_stm *stm3 = L_Jump(end_label);
        emit_irs.push_back(stm3);

        L_stm *stm4 = L_Label(false_label);
        emit_irs.push_back(stm4);
        L_stm *stm5 = L_Store(AS_Operand_Const(0), tmp);
        emit_irs.push_back(stm5);
        L_stm *stm6 = L_Jump(end_label);
        emit_irs.push_back(stm6);

        L_stm *stm7 = L_Label(end_label);
        emit_irs.push_back(stm7);
        AS_operand* ret = AS_Operand_Temp(Temp_newtemp_int());
        L_stm *stm8 = L_Load(ret, AS_Operand_Temp(tmp->u.TEMP));
        emit_irs.push_back(stm8);

        return ret;
    }
        break;
    default:
        break;
    }
}

AS_operand* ast2llvmLeftVal(aA_leftVal l)
{
    switch (l->kind)
    {
    case A_leftValType::A_varValKind:
    {
        if(localVarMap.find(*l->u.id) != localVarMap.end()) // local var
        {
            return AS_Operand_Temp(localVarMap[*l->u.id]);
        }
        else // global var
        {
            return AS_Operand_Name(globalVarMap[*l->u.id]);
        }
    }
        break;
    case A_leftValType::A_arrValKind:
    {
        AS_operand* index = ast2llvmIndexExpr(l->u.arrExpr->idx);
        AS_operand* base = ast2llvmLeftVal(l->u.arrExpr->arr);
        if(base->kind == OperandKind::NAME) {
            if(base->u.NAME->type == TempType::INT_PTR) {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_int_ptr(0));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else if(base->u.NAME->type == TempType::STRUCT_PTR) {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_struct_ptr(0, base->u.NAME->structname));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else if(base->u.NAME->type == TempType::STRUCT_TEMP) {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_struct_ptr(0, base->u.NAME->structname));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else {
                assert(0);
            }
        }
        else if(base->kind == OperandKind::TEMP) {
            if(base->u.TEMP->type == TempType::INT_PTR)
            {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_int_ptr(0));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else if(base->u.TEMP->type == TempType::STRUCT_PTR)
            {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_struct_ptr(0, base->u.TEMP->structname));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else
            {
                assert(0);
            }
        }
        else {
            assert(0);
        }
    }
        break;
    case A_leftValType::A_memberValKind:
    {
        AS_operand* base = ast2llvmLeftVal(l->u.memberExpr->structId);
        if(base->kind == OperandKind::NAME) {
            MemberInfo info = structInfoMap[base->u.NAME->structname].memberinfos[*l->u.memberExpr->memberId];
            AS_operand* index = AS_Operand_Const(info.offset);
            if(info.def.kind == TempType::INT_PTR) {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_int_ptr(info.def.len));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else if(info.def.kind == TempType::STRUCT_PTR) {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_struct_ptr(info.def.len, info.def.structname));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else if(info.def.kind == TempType::INT_TEMP) {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_int_ptr(0));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else {
                assert(0);
            }
        }
        else if(base->kind == OperandKind::TEMP) {
            MemberInfo info = structInfoMap[base->u.TEMP->structname].memberinfos[*l->u.memberExpr->memberId];
            AS_operand* index = AS_Operand_Const(info.offset);
            if(info.def.kind == TempType::INT_PTR)
            {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_int_ptr(info.def.len));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else if(info.def.kind == TempType::STRUCT_PTR)
            {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_struct_ptr(info.def.len, info.def.structname));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else if(info.def.kind == TempType::INT_TEMP) {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_int_ptr(0));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
            else if(info.def.kind == TempType::STRUCT_TEMP)
            {
                AS_operand* tmp = AS_Operand_Temp(Temp_newtemp_struct_ptr(info.def.len, info.def.structname));
                L_stm *stm = L_Gep(tmp, base, index);
                emit_irs.push_back(stm);
                return tmp;
            }
        }
        else {
            assert(0);
        }
    }
        break;
    default:
        break;
    }
}

AS_operand* ast2llvmIndexExpr(aA_indexExpr index)
{
    if(index->kind == A_indexExprKind::A_numIndexKind)
    {
        return AS_Operand_Const(index->u.num);
    }
    else if(index->kind == A_indexExprKind::A_idIndexKind)
    {
        AS_operand* ptr;
        if(localVarMap.find(*index->u.id) != localVarMap.end())
        {
            ptr = AS_Operand_Temp(localVarMap[*index->u.id]);
        }
        else
        {
           ptr = AS_Operand_Name(globalVarMap[*index->u.id]);
        }
        
        AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
        L_stm *stm = L_Load(dst, ptr);
        emit_irs.push_back(stm);
        return dst;
    }
    else
    {
        assert(0);
    }
}

AS_operand* ast2llvmBoolExpr(aA_boolExpr b,Temp_label *true_label,Temp_label *false_label)
{
    if(b->kind == A_boolExprType::A_boolBiOpExprKind)
    {
        ast2llvmBoolBiOpExpr(b->u.boolBiOpExpr, true_label, false_label);
    }
    else 
    {
        ast2llvmBoolUnit(b->u.boolUnit, true_label, false_label);
    }
    return nullptr;
}

void ast2llvmBoolBiOpExpr(aA_boolBiOpExpr b,Temp_label *true_label,Temp_label *false_label)
{
    if(b->op == A_boolBiOp::A_and) {
        Temp_label *mid_label = Temp_newlabel();
        AS_operand *left = ast2llvmBoolExpr(b->left, mid_label, false_label);
        L_stm *stm = L_Label(mid_label);
        emit_irs.push_back(stm);
        AS_operand *right = ast2llvmBoolExpr(b->right, true_label, false_label);
        
    }
    else if(b->op == A_boolBiOp::A_or) {
        Temp_label *mid_label = Temp_newlabel();
        AS_operand *left = ast2llvmBoolExpr(b->left, true_label, mid_label);
        L_stm *stm = L_Label(mid_label);
        emit_irs.push_back(stm);
        AS_operand *right = ast2llvmBoolExpr(b->right, true_label, false_label);
    }
}

void ast2llvmBoolUnit(aA_boolUnit b,Temp_label *true_label,Temp_label *false_label)
{
    if(b->kind == A_boolUnitType::A_boolExprKind) {
        AS_operand *cond = ast2llvmBoolExpr(b->u.boolExpr, true_label, false_label);
        // L_stm *stm = L_Cjump(cond, true_label, false_label);
        // emit_irs.push_back(stm);
    }
    else if(b->kind == A_boolUnitType::A_comOpExprKind) {
        ast2llvmComOpExpr(b->u.comExpr, true_label, false_label);
    }
    else if(b->kind == A_boolUnitType::A_boolUOpExprKind){
        ast2llvmBoolUnit(b->u.boolUOpExpr->cond, false_label, true_label);
        // L_stm *stm = L_Cjump(cond, false_label, true_label);
        // emit_irs.push_back(stm);
    }
}

void ast2llvmComOpExpr(aA_comExpr c,Temp_label *true_label,Temp_label *false_label)
{
    AS_operand* left = ast2llvmExprUnit(c->left);
    AS_operand* right = ast2llvmExprUnit(c->right);
    switch (c->op)
    {
    case A_comOp::A_eq:
    {
        AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
        L_stm *stm = L_Cmp(L_relopKind::T_eq, left, right, dst);
        emit_irs.push_back(stm);
        L_stm *stm2 = L_Cjump(dst, true_label, false_label);
        emit_irs.push_back(stm2);
    }
        break;
    case A_comOp::A_ne:
    {
        AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
        L_stm *stm = L_Cmp(L_relopKind::T_ne, left, right, dst);
        emit_irs.push_back(stm);
        L_stm *stm2 = L_Cjump(dst, true_label, false_label);
        emit_irs.push_back(stm2);
    }
        break;
    case A_comOp::A_lt:
    {
        AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
        L_stm *stm = L_Cmp(L_relopKind::T_lt, left, right, dst);
        emit_irs.push_back(stm);
        L_stm *stm2 = L_Cjump(dst, true_label, false_label);
        emit_irs.push_back(stm2);
    }
        break;
    case A_comOp::A_le:
    {
        AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
        L_stm *stm = L_Cmp(L_relopKind::T_le, left, right, dst);
        emit_irs.push_back(stm);
        L_stm *stm2 = L_Cjump(dst, true_label, false_label);
        emit_irs.push_back(stm2);
    }
        break;
    case A_comOp::A_gt:
    {
        AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
        L_stm *stm = L_Cmp(L_relopKind::T_gt, left, right, dst);
        emit_irs.push_back(stm);
        L_stm *stm2 = L_Cjump(dst, true_label, false_label);
        emit_irs.push_back(stm2);
    }
        break;
    case A_comOp::A_ge:
    {
        AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
        L_stm *stm = L_Cmp(L_relopKind::T_ge, left, right, dst);
        emit_irs.push_back(stm);
        L_stm *stm2 = L_Cjump(dst, true_label, false_label);
        emit_irs.push_back(stm2);
    }
        break;
    default:
        break;
    }
}

AS_operand* ast2llvmArithBiOpExpr(aA_arithBiOpExpr a)
{
    AS_operand* left = ast2llvmArithExpr(a->left);
    AS_operand* right = ast2llvmArithExpr(a->right);
    AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
    switch (a->op)
    {
    case A_arithBiOp::A_add:
    {
        L_stm *stm = L_Binop(L_binopKind::T_plus, left, right, dst);
        emit_irs.push_back(stm);
    }
        break;
    case A_arithBiOp::A_sub:
    {
        L_stm *stm = L_Binop(L_binopKind::T_minus, left, right, dst);
        emit_irs.push_back(stm);
    }
        break;
    case A_arithBiOp::A_mul:
    {
        L_stm *stm = L_Binop(L_binopKind::T_mul, left, right, dst);
        emit_irs.push_back(stm);
    }
        break;
    case A_arithBiOp::A_div:
    {
        L_stm *stm = L_Binop(L_binopKind::T_div, left, right, dst);
        emit_irs.push_back(stm);
    }
        break;
    }
    return dst;
}

AS_operand* ast2llvmArithUExpr(aA_arithUExpr a)
{
    AS_operand* expr = ast2llvmExprUnit(a->expr);
    AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
    L_stm *stm = L_Binop(L_binopKind::T_minus, AS_Operand_Const(0), expr, dst);
    emit_irs.push_back(stm);

    return dst;
}

AS_operand* ast2llvmArithExpr(aA_arithExpr a)
{
    if(a->kind == A_arithBiOpExprKind)
    {
        return ast2llvmArithBiOpExpr(a->u.arithBiOpExpr);
    }
    else if(a->kind == A_exprUnitKind)
    {
        return ast2llvmExprUnit(a->u.exprUnit);
    }
    else
    {
        assert(0);
    }
}

AS_operand* ast2llvmExprUnit(aA_exprUnit e)
{
    switch (e->kind)
    {
    case A_exprUnitType::A_numExprKind:
    {
        return AS_Operand_Const(e->u.num);
    }
        break;
    case A_exprUnitType::A_idExprKind:
    {
        if(localVarMap.find(*e->u.id) != localVarMap.end()) // local var
        {
            AS_operand* ptr = AS_Operand_Temp(localVarMap[*e->u.id]);
            AS_operand* dst;
            if(ptr->u.TEMP->type == TempType::INT_PTR)
            {
                if(ptr->u.TEMP->len == 0)
                {
                    dst = AS_Operand_Temp(Temp_newtemp_int());
                    L_stm *stm = L_Load(dst, ptr);
                    emit_irs.push_back(stm);
                }
                else
                {
                    dst = ptr;
                }
            }
            else if(ptr->u.TEMP->type == TempType::STRUCT_PTR) 
            {
                dst = ptr;
            }
            else if(ptr->u.TEMP->type == TempType::INT_TEMP) // should never enter
            {
                dst = ptr;
            }
            else {
                assert(0);
            }
            return dst;
        }
        else // global var
        {
            AS_operand* ptr = AS_Operand_Name(globalVarMap[*e->u.id]);
            AS_operand* dst;
            if(ptr->u.NAME->type == TempType::INT_PTR)
            {
                if(ptr->u.NAME->len == 0)
                {
                    dst = AS_Operand_Temp(Temp_newtemp_int());
                    L_stm *stm = L_Load(dst, ptr);
                    emit_irs.push_back(stm);
                }
                else
                {
                    dst = ptr;
                }
            }
            else if(ptr->u.NAME->type == TempType::INT_TEMP) { //global temp is actually a ptr
                dst = AS_Operand_Temp(Temp_newtemp_int());
                L_stm *stm = L_Load(dst, ptr);
                emit_irs.push_back(stm);
            }
            else
            {
                dst = ptr;
            }
            return dst;
        }
    }
        break;
    case A_exprUnitType::A_arrayExprKind:
    {
        AS_operand* index = ast2llvmIndexExpr(e->u.arrayExpr->idx);
        AS_operand* base = ast2llvmLeftVal(e->u.arrayExpr->arr);
        if(base->kind == OperandKind::NAME) {
            if(base->u.NAME->type == TempType::INT_PTR) {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_int_ptr(0));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);

                AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
                L_stm *stm2 = L_Load(dst, ptr);
                emit_irs.push_back(stm2);
                return dst;
            }
            else if(base->u.NAME->type == TempType::STRUCT_PTR) {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_struct_ptr(0, base->u.NAME->structname));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);
                // weird
                // AS_operand* dst = AS_Operand_Temp(Temp_newtemp_struct_ptr(0, base->u.NAME->structname));
                // L_stm *stm2 = L_Load(dst, ptr);
                // emit_irs.push_back(stm2);
                return ptr;
            }
            else {
                assert(0);
            }
        }
        else if(base->kind == OperandKind::TEMP) {
            if(base->u.TEMP->type == TempType::INT_PTR)
            {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_int_ptr(0));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);

                AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
                L_stm *stm2 = L_Load(dst, ptr);
                emit_irs.push_back(stm2);
                return dst;
            }
            else if(base->u.TEMP->type == TempType::STRUCT_PTR)
            {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_struct_ptr(0, base->u.TEMP->structname));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);
                // weird
                // AS_operand* dst = AS_Operand_Temp(Temp_newtemp_struct_ptr(0, base->u.TEMP->structname));
                // L_stm *stm2 = L_Load(dst, ptr);
                // emit_irs.push_back(stm2);
                return ptr;
            }
            else
            {
                assert(0);
            }
        }
        else {
            assert(0);
        }
    }
        break;
    case A_exprUnitType::A_memberExprKind:
    {
        AS_operand* base = ast2llvmLeftVal(e->u.memberExpr->structId);
        if(base->kind == OperandKind::NAME) {
            MemberInfo info = structInfoMap[base->u.NAME->structname].memberinfos[*e->u.memberExpr->memberId];
            AS_operand* index = AS_Operand_Const(info.offset);
            if(info.def.kind == TempType::INT_PTR) {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_int_ptr(info.def.len));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);

                // AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
                // L_stm *stm2 = L_Load(dst, ptr);
                // emit_irs.push_back(stm2);
                return ptr;
                
            }
            else if(info.def.kind == TempType::STRUCT_PTR) {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_struct_ptr(info.def.len, info.def.structname));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);
                //werid
                // AS_operand* dst = AS_Operand_Temp(Temp_newtemp_struct_ptr(info.def.len, info.def.structname));
                // L_stm *stm2 = L_Load(dst, ptr);
                // emit_irs.push_back(stm2);
                return ptr;
            }
            else if(info.def.kind == TempType::INT_TEMP) {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_int_ptr(0));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);

                AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
                L_stm *stm2 = L_Load(dst, ptr);
                emit_irs.push_back(stm2);
                return dst;
            }
            else {
                assert(0);
            }
        }
        else if(base->kind == OperandKind::TEMP) {
            MemberInfo info = structInfoMap[base->u.TEMP->structname].memberinfos[*e->u.memberExpr->memberId];
            AS_operand* index = AS_Operand_Const(info.offset);
            if(info.def.kind == TempType::INT_PTR) {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_int_ptr(info.def.len));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);

                // AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
                // L_stm *stm2 = L_Load(dst, ptr);
                // emit_irs.push_back(stm2);
                return ptr;
                
            }
            else if(info.def.kind == TempType::STRUCT_PTR) {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_struct_ptr(info.def.len, info.def.structname));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);
                //werid
                // AS_operand* dst = AS_Operand_Temp(Temp_newtemp_struct_ptr(info.def.len, info.def.structname));
                // L_stm *stm2 = L_Load(dst, ptr);
                // emit_irs.push_back(stm2);
                return ptr;
            }
            else if(info.def.kind == TempType::INT_TEMP) {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_int_ptr(0));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);

                AS_operand* dst = AS_Operand_Temp(Temp_newtemp_int());
                L_stm *stm2 = L_Load(dst, ptr);
                emit_irs.push_back(stm2);
                return dst;
            }
            else if(info.def.kind == TempType::STRUCT_TEMP) {
                AS_operand* ptr = AS_Operand_Temp(Temp_newtemp_struct_ptr(info.def.len, info.def.structname));
                L_stm *stm = L_Gep(ptr, base, index);
                emit_irs.push_back(stm);
                return ptr;
            }
        }
        else {
            assert(0);
        }
    }
        break;
    case A_exprUnitType::A_fnCallKind:
    {
        vector<AS_operand*> args;
        for(const aA_rightVal arg : e->u.callExpr->vals)
        {
            args.push_back(ast2llvmRightVal(arg));
        }
        AS_operand* ret = AS_Operand_Temp(Temp_newtemp_int());
        L_stm *stm = L_Call(*e->u.callExpr->fn, ret, args);
        emit_irs.push_back(stm);
        return ret;
    }
        break;
    case A_exprUnitType::A_arithUExprKind:
    {
        return ast2llvmArithUExpr(e->u.arithUExpr);
    }
        break;
    case A_exprUnitType::A_arithExprKind:
    {
        return ast2llvmArithExpr(e->u.arithExpr);
    }
        break;
    }
}

LLVMIR::L_func* ast2llvmFuncBlock(Func_local *f)
{
    list<LLVMIR::L_stm *> tmp_list = list<LLVMIR::L_stm *>();
    list<L_block*> blocks = list<L_block*>();
    for(L_stm* stm : f->irs)
    {           
        if(stm->type == L_StmKind::T_LABEL) {
            if(!tmp_list.empty()) {
                L_block* block = LLVMIR::L_Block(tmp_list);
                blocks.push_back(block);
                tmp_list.clear();
            }
        }
        tmp_list.push_back(stm);
    }
    if(!tmp_list.empty()) {
        L_block* block = LLVMIR::L_Block(tmp_list);
        blocks.push_back(block);
        tmp_list.clear();
    }
    return new L_func(f->name, f->ret, f->args, blocks);
}

void ast2llvm_moveAlloca(LLVMIR::L_func *f)
{
    for (auto it = next(f->blocks.begin()); it != f->blocks.end(); it++)
    {
        L_block *block = *it;
        for (auto it2 = block->instrs.begin(); it2 != block->instrs.end(); it2++)
        {
            L_stm *stm = *it2;
            if (stm->type == L_StmKind::T_ALLOCA)
            {
                L_block* block = f->blocks.front();
                block->instrs.insert(next(block->instrs.begin()), stm);
                it2 = block->instrs.erase(it2);
                it2--;
            }
        }
    }
    
}