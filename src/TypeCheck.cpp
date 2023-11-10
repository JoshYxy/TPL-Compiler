#include "TypeCheck.h"

// maps to store the type information. Feel free to design new data structures if you need.
typeMap g_token2Type; // global token ids to type
typeMap cur_token2Type; // current func params and local var
typeMap funcparam_token2Type; // func params token ids to type

intMap g_token2ArrLen;
intMap cur_token2ArrLen;
intMap funcStat;

paramMemberMap func2Param;
// paramArrMap func2ParamArr;
paramMemberMap struct2Members;

string cur_fn; // to check for ret type

// private util functions. You can use these functions to help you debug.
void error_print(std::ostream* out, A_pos p, string info)
{
    *out << "Typecheck error in line " << p->line << ", col " << p->col << ": " << info << std::endl;
    exit(0);
}


void print_token_map(typeMap* map){
    for(auto it = map->begin(); it != map->end(); it++){
        std::cout << it->first << " : ";
        switch (it->second->type)
        {
        case A_dataType::A_nativeTypeKind:
            switch (it->second->u.nativeType)
            {
            case A_nativeType::A_intTypeKind:
                std::cout << "int";
                break;
            default:
                break;
            }
            break;
        case A_dataType::A_structTypeKind:
            std::cout << *(it->second->u.structType);
            break;
        default:
            break;
        }
        std::cout << std::endl;
    }
}


// public functions
// This is the entrace of this file.
void check_Prog(std::ostream* out, aA_program p)
{
    // p is the root of AST tree.
    for (auto ele : p->programElements)
    {
    /*
        Write your code here.
        Hint: 
        1. Design the order of checking the program elements to meet the requirements that funtion declaration and global variable declaration can be used anywhere in the program.

        2. Many types of statements indeed collapse to some same units, so a good abstract design will help you reduce the amount of your code.
    */ 
        switch (ele->kind)
        {
        case A_programElementType::A_programVarDeclStmtKind: //global var decl
            {
                aA_varDeclStmt vd = ele->u.varDeclStmt;
                check_VarDecl(out, vd);
                add_VarDecl(vd, true, nullptr);
                // if (vd->kind == A_varDeclStmtType::A_varDeclKind){
                //     if (vd->u.varDecl->kind == A_varDeclType::A_varDeclScalarKind) {
                //         aA_varDeclScalar vds = vd->u.varDecl->u.declScalar;
                //         string* id = vds->id;
                //         aA_type type = vds->type;
                        
                //         g_token2Type.insert(make_pair(*id, type));
                //     }
                //     if (vd->u.varDecl->kind == A_varDeclType::A_varDeclArrayKind) {
                //         aA_varDeclArray vda = vd->u.varDecl->u.declArray;
                //         string* id = vda->id;
                //         aA_type type = vda->type;
                //         int len = vda->len;

                //         g_token2ArrLen.insert(make_pair(*id, len));
                //         g_token2Type.insert(make_pair(*id, type));
                //     }
                    
                // }
                // else if (vd->kind == A_varDeclStmtType::A_varDefKind){
                //     if (vd->u.varDef->kind == A_varDefType::A_varDefScalarKind) {
                //         aA_varDefScalar vds = vd->u.varDef->u.defScalar;
                //         string* id = vds->id;
                //         aA_type type = vds->type;

                //         g_token2Type.insert(make_pair(*id, type));
                //     }
                //     if (vd->u.varDef->kind == A_varDefType::A_varDefArrayKind) {
                //         aA_varDefArray vda = vd->u.varDef->u.defArray;
                //         string* id = vda->id;
                //         aA_type type = vda->type;
                //         int len = vda->len;

                //         g_token2ArrLen.insert(make_pair(*id, len));
                //         g_token2Type.insert(make_pair(*id, type));
                //     }
                // }
                break;
            }
        case A_programElementType::A_programStructDefKind:
            {
                check_StructDef(out, ele->u.structDef);
                break;
            }
        default:
            break;
        }   
    }
    // print_token_map(&g_token2Type);
    // first check all declstmt, check def in next turn
    for (auto ele : p->programElements)
    {
        switch (ele->kind)
        {
        case A_programElementType::A_programFnDeclStmtKind: 
            {
                check_FnDeclStmt(out, ele->u.fnDeclStmt);
                break;
            }
        case A_programElementType::A_programFnDefKind: 
            {
                check_FnDef(out, ele->u.fnDef);
                break;
            }
        default:
            break;
        }  
    }
    
    for (auto ele : p->programElements)
    {
            
    }

    (*out) << "Typecheck passed!" << std::endl;
    return;
}

void add_VarDecl(aA_varDeclStmt vd, bool global, vector<string>* s) {
    string *id;
    aA_type type;
    int len = 0;
    if (vd->kind == A_varDeclStmtType::A_varDeclKind){
        if (vd->u.varDecl->kind == A_varDeclType::A_varDeclScalarKind) {
            aA_varDeclScalar vds = vd->u.varDecl->u.declScalar;
            id = vds->id;
            type = vds->type;

        }
        if (vd->u.varDecl->kind == A_varDeclType::A_varDeclArrayKind) {
            aA_varDeclArray vda = vd->u.varDecl->u.declArray;
            id = vda->id;
            type = vda->type;
            len = vda->len;
        }
        
    }
    else if (vd->kind == A_varDeclStmtType::A_varDefKind){
        if (vd->u.varDef->kind == A_varDefType::A_varDefScalarKind) {
            aA_varDefScalar vds = vd->u.varDef->u.defScalar;
            id = vds->id;
            type = vds->type;        
        }
        if (vd->u.varDef->kind == A_varDefType::A_varDefArrayKind) {
            aA_varDefArray vda = vd->u.varDef->u.defArray;
            id = vda->id;
            type = vda->type;
            len = vda->len;
        }
    }
        
    if(global) {
        if(len > 0)  g_token2ArrLen.insert(make_pair(*id, len));
        g_token2Type.insert(make_pair(*id, type));
    }
    else {
        if(s) (*s).emplace_back(*id);
        cur_token2Type.insert(make_pair(*id, type));
        if(len > 0) cur_token2ArrLen.insert(make_pair(*id, len));
    }
}

void check_VarDecl(std::ostream* out, aA_varDeclStmt vd)//only check, not for map maintenance due to submap
{
    // variable declaration statement 
    if (!vd)
        return;
    string name;
    if (vd->kind == A_varDeclStmtType::A_varDeclKind){
        // if declaration only 
        // Example:
        //   let a:int;
        //   let a[5]:int;
        
        /* write your code here*/
        if (vd->u.varDecl->kind == A_varDeclType::A_varDeclScalarKind) {
            aA_varDeclScalar vds = vd->u.varDecl->u.declScalar;
            string* id = vds->id;

            if (g_token2Type.find(*id) != g_token2Type.end()){
                error_print(out, vds->pos, "Var " + *id + " exists in global");
            }
            if (cur_token2Type.find(*id) != cur_token2Type.end()){
                error_print(out, vds->pos, "Var " + *id + " exists in current scope");
            }

            aA_type type = vds->type;
            struct_type_check(out, vds->pos, type);

        }
        if (vd->u.varDecl->kind == A_varDeclType::A_varDeclArrayKind) {
            aA_varDeclArray vda = vd->u.varDecl->u.declArray;
            string* id = vda->id;

            if (g_token2Type.find(*id) != g_token2Type.end()){
                error_print(out, vda->pos, "Arr " + *id + " exists in global");
            }
            if (cur_token2Type.find(*id) != cur_token2Type.end()){
                error_print(out, vda->pos, "Arr " + *id + " exists in current scope");
            }
            
            aA_type type = vda->type;
            struct_type_check(out, vda->pos, type);
        }
            
    }
    else if (vd->kind == A_varDeclStmtType::A_varDefKind){
        // if both declaration and initialization 
        // Example:
        //   let a:int = 5;
        
        /* write your code here */
        if(vd->u.varDef->kind == A_varDefType::A_varDefScalarKind) {
            aA_varDefScalar vds = vd->u.varDef->u.defScalar;
            string* id = vds->id;
            aA_type var_type = vds->type;

            if (g_token2Type.find(*id) != g_token2Type.end()){
                error_print(out, vds->pos, "Var " + *id + "  exists in global");
            }
            if (cur_token2Type.find(*id) != cur_token2Type.end()){
                error_print(out, vds->pos, "Var " + *id + "  exists in current scope");
            }

            struct_type_check(out, vds->pos, var_type);
            assign_type_check(out, var_type, vds->val);
            assign_arr_check(out, false, vds->val);
        }
        if(vd->u.varDef->kind == A_varDefType::A_varDefArrayKind) {
            aA_varDefArray vda = vd->u.varDef->u.defArray;
            string* id = vda->id;
            aA_type arr_type = vda->type;
            int len = vda->len;
            
            if (g_token2Type.find(*id) != g_token2Type.end()){
                error_print(out, vda->pos, "Arr " + *id + " exists in global");
            }
            if (cur_token2Type.find(*id) != cur_token2Type.end()){
                error_print(out, vda->pos, "Arr " + *id + " exists in current scope");
            }
            
            struct_type_check(out, vda->pos, arr_type);
            for(auto rv : vda->vals){
                assign_type_check(out, arr_type, rv);
                assign_arr_check(out, false, rv);
            }
        }
    }
    return;
}
void struct_type_check(std::ostream* out, A_pos pos, aA_type type)
{
    if(type == nullptr) return;
    if(type->type == A_dataType::A_structTypeKind) {
        //chekc struct define
        string* type_name = type->u.structType;
        if (struct2Members.find(*type_name) == struct2Members.end()) {
            error_print(out, pos, "Type " + *type_name + " not defined");
        }
    }
    return;
}

string get_type(aA_type type) {
    if(type == nullptr) return "null";
    if(type->type == A_dataType::A_nativeTypeKind) {
        return "int";
    }
    return type->u.structType != nullptr ? *(type->u.structType) : "null";
}

void assign_arr_check(std::ostream* out, bool isArr, aA_rightVal val) {
    bool check_res = false;

    if(val->kind == A_rightValType::A_arithExprValKind) {
        aA_arithExpr arExp = val->u.arithExpr;
        if(arExp->kind == A_arithExprType::A_exprUnitKind) {
            check_res = check_ExprUnitArr(arExp->u.exprUnit);    
        }
    }
    if(isArr != check_res) {
        error_print(out, val->pos, "Array and Scalar not match");
    }
}

void assign_type_check(std::ostream* out, aA_type type, aA_rightVal val) //check whether type of 'type' and 'val' matches, also val validity
{
    if(val->kind == A_rightValType::A_boolExprValKind) {
        error_print(out, val->pos, "Can't be assigned a bool value");
    }

    if(val->kind == A_rightValType::A_arithExprValKind) {
        aA_arithExpr arExp = val->u.arithExpr;
        if(arExp->kind == A_arithExprType::A_arithBiOpExprKind) {
            check_ArithBiOpExpr(out, arExp->u.arithBiOpExpr);
            if(get_type(type) != "int") {
                error_print(out, arExp->pos, "Right-value's type inconsistent with desired type");
            }
        }
        if(arExp->kind == A_arithExprType::A_exprUnitKind) {
            aA_type rType = check_ExprUnit(out, arExp->u.exprUnit);
            // *out << "r_type:"+get_type(rType)+" type:"+get_type(type)+"\n";
            if(get_type(type) != get_type(rType)) {
                error_print(out, arExp->pos, "Right-value's type inconsistent with desired type");
            }
        }
    }

    // error_print(out, val->pos, "Can't confirm right-value's type");
    return;
}

void check_StructDef(std::ostream* out, aA_structDef sd)
{
    if (!sd)
        return;
    // structure definition
    // Example:
    //      struct A{
    //          a:int;
    //          b:int;
    //      }
    
    /* write your code here */
    
    string id = *(sd->id);
    if (g_token2Type.find(id) != g_token2Type.end()){
        error_print(out, sd->pos, "id: " + id + " exists");
    }
    
    vector<aA_varDecl> vars = sd->varDecls;
    vector<aA_varDecl>* vp = new vector<aA_varDecl>(vars);
    aA_type ph = aA_Type(A_StructType(nullptr, "")); // quite strange

    struct2Members.insert(make_pair(id, vp)); // add first for struct in struct
    g_token2Type.insert(make_pair(id, ph));// struct name can't coincide with a var name
    for(auto var : vars) {
        if (var->kind == A_varDeclType::A_varDeclScalarKind) {
            aA_varDeclScalar vds = var->u.declScalar;
            string* id = vds->id;
            aA_type type = vds->type;
            struct_type_check(out, vds->pos, type);
        }
        if (var->kind == A_varDeclType::A_varDeclArrayKind) {
            aA_varDeclArray vda = var->u.declArray;
            string* id = vda->id;
            aA_type type = vda->type;
            struct_type_check(out, vda->pos, type);
        }
    }
    return;
}


void check_FnDecl(std::ostream* out, aA_fnDecl fd)
{
    // Example:
    //      fn main(a:int, b:int)->int
    if (!fd)
        return;     
    /*  
        write your code here
        Hint: you may need to check if the function is already declared
    */
    string name = *(fd->id);
    aA_type type = fd->type;
    if(type != nullptr) struct_type_check(out, type->pos, type);

    if (g_token2Type.find(name) == g_token2Type.end()) { //not already declared or defined
        g_token2Type.insert(make_pair(name, type));
    }
    else {
        if (get_type(g_token2Type.at(name)) != get_type(type)) { // check return value
            error_print(out, fd->pos, "return type different in function " + name);
        }
        // check params

        vector<aA_varDecl> new_vars = fd->paramDecl->varDecls;
        vector<aA_varDecl> vars = *(func2Param.at(name));
       
        aA_type cur;
        aA_type new_cur;
        string name;
        string new_name;
        if(new_vars.size() != vars.size()) {
            error_print(out, fd->pos, "Param number incompatible between define and declare");
        }
        for(int i = 0; i < new_vars.size(); i++) {
            if (vars[i]->kind != new_vars[i]->kind) {
                error_print(out, fd->pos, "Param mismatch between define and declare");
            }
            if (vars[i]->kind == A_varDeclType::A_varDeclScalarKind) {
                cur = vars[i]->u.declScalar->type;
                new_cur = new_vars[i]->u.declScalar->type;
                if(get_type(cur) != get_type(new_cur)) {
                    error_print(out, fd->pos, "Param type mismatch between define and declare");
                }
                
                name = *vars[i]->u.declScalar->id;
                new_name = *new_vars[i]->u.declScalar->id;
                if(name != new_name) {
                    error_print(out, fd->pos, "Param name mismatch between define and declare");
                }
            }
            if (vars[i]->kind == A_varDeclType::A_varDeclArrayKind) {
                cur = vars[i]->u.declArray->type;
                new_cur = new_vars[i]->u.declArray->type;
                if(get_type(cur) != get_type(new_cur)) {
                    error_print(out, fd->pos, "Param type mismatch between define and declare");
                }
                                
                name = *vars[i]->u.declArray->id;
                new_name = *new_vars[i]->u.declArray->id;
                if(name != new_name) {
                    error_print(out, fd->pos, "Param name mismatch between define and declare");
                }
            }
        }

    }
    vector<bool>* isArr = new vector<bool>;
    if (func2Param.find(name) == func2Param.end()) { 
        vector<aA_varDecl> vars = fd->paramDecl->varDecls;
        for(auto var : vars) {
            if (var->kind == A_varDeclType::A_varDeclScalarKind) {
                aA_varDeclScalar vds = var->u.declScalar;
                string* id = vds->id;
                aA_type type = vds->type;
                struct_type_check(out, vds->pos, type);
                isArr->emplace_back(false);
            }
            if (var->kind == A_varDeclType::A_varDeclArrayKind) {
                aA_varDeclArray vda = var->u.declArray;
                string* id = vda->id;
                aA_type type = vda->type;
                struct_type_check(out, vda->pos, type);
                isArr->emplace_back(true);
            }
        }
        vector<aA_varDecl>* vp = new vector<aA_varDecl>(vars);
        
        func2Param.insert(make_pair(name, vp));
        // func2ParamArr.insert(make_pair(name, isArr));
    }

    return;
}


void check_FnDeclStmt(std::ostream* out, aA_fnDeclStmt fd)
{
    // Example:
    //      fn main(a:int, b:int)->int;
    if (!fd)
        return;

    string name = *(fd->fnDecl->id);
    if (funcStat.find(name) != funcStat.end()) {
        if ((funcStat.at(name) & 1) != 0) { //declared
            error_print(out, fd->pos, "function has already been declared");
        }
        if ((funcStat.at(name) & 2) != 0) { //defined
            error_print(out, fd->pos, "function has already been defined");
        }
    }

    check_FnDecl(out, fd->fnDecl);
    // update function status
    if (funcStat.find(name) != funcStat.end()) {
        funcStat.at(name) = funcStat.at(name) & 1;
    } 
    else {
        funcStat.insert(make_pair(name, 1));
    }
    return;
}


void check_FnDef(std::ostream* out, aA_fnDef fd)
{
    // Example:
    //      fn main(a:int, b:int)->int{
    //          let c:int;
    //          c = a + b;
    //          return c;
    //      }
    if (!fd)
        return;
    /*  
        write your code here 
        Hint: you may pay attention to the function parameters, local variables and global variables.
    */
    string name = *(fd->fnDecl->id);
    if (funcStat.find(name) != funcStat.end()) {
        if ((funcStat.at(name) & 2) != 0) { //defined
            error_print(out, fd->pos, "function has already been defined");
        }
    }
   
    check_FnDecl(out, fd->fnDecl);
    cur_fn = name;
    // add param to cur ids
    vector<aA_varDecl>* v = func2Param.at(name);
    for(aA_varDecl var : *v) {

        if (var->kind == A_varDeclType::A_varDeclScalarKind) {
            cur_token2Type.insert(make_pair(*(var->u.declScalar->id), var->u.declScalar->type));
        }
        if (var->kind == A_varDeclType::A_varDeclArrayKind) {
            cur_token2Type.insert(make_pair(*(var->u.declArray->id), var->u.declArray->type));  
            cur_token2ArrLen.insert(make_pair(*(var->u.declArray->id), var->u.declArray->len));
        }

    }

    check_CodeblockStmtList(out, fd->stmts);
    // for(aA_codeBlockStmt s : fd->stmts){
    //     check_CodeblockStmt(out, s);
    // }
    // remove param from cur ids
    for(aA_varDecl var : *v) {

        if (var->kind == A_varDeclType::A_varDeclScalarKind) {
            cur_token2Type.erase(*(var->u.declScalar->id));
        }
        if (var->kind == A_varDeclType::A_varDeclArrayKind) {
            cur_token2Type.erase(*(var->u.declArray->id));
            cur_token2ArrLen.erase(*(var->u.declArray->id));  
        }

    }

    // update function status
    
    if (funcStat.find(name) != funcStat.end()) {
        funcStat.at(name) = 2;
    } 
    else {
        funcStat.insert(make_pair(name, 2));
    }

    return;
}

void check_CodeblockStmtList(std::ostream* out, vector<aA_codeBlockStmt> blocks){

    vector<string> sub_token;

    for(aA_codeBlockStmt s : blocks){
        check_CodeblockStmt(out, s, &sub_token);
    }

    for(auto s : sub_token) {
        cur_token2Type.erase(s);
        cur_token2ArrLen.erase(s);
    }
}

void check_CodeblockStmt(std::ostream* out, aA_codeBlockStmt cs, vector<string>* sub_token){
    if(!cs)
        return;
    switch (cs->kind)
    {
    case A_codeBlockStmtType::A_varDeclStmtKind:
        check_VarDecl(out, cs->u.varDeclStmt);
        add_VarDecl(cs->u.varDeclStmt, false, sub_token);
        break;
    case A_codeBlockStmtType::A_assignStmtKind:
        check_AssignStmt(out, cs->u.assignStmt);
        break;
    case A_codeBlockStmtType::A_ifStmtKind:
        check_IfStmt(out, cs->u.ifStmt);
        break;
    case A_codeBlockStmtType::A_whileStmtKind:
        check_WhileStmt(out, cs->u.whileStmt);
        break;
    case A_codeBlockStmtType::A_callStmtKind:
        check_CallStmt(out, cs->u.callStmt);
        break;
    case A_codeBlockStmtType::A_returnStmtKind:
        check_ReturnStmt(out, cs->u.returnStmt);
        break;
    default:
        break;
    }
    return;
}


void check_AssignStmt(std::ostream* out, aA_assignStmt as){
    if(!as)
        return;
    string name;
    aA_type left_type;
    assign_arr_check(out, false, as->rightVal); // right value must not be array
    switch (as->leftVal->kind)
    {
        case A_leftValType::A_varValKind:{
            /* write your code here */
            name = *(as->leftVal->u.id);
            left_type = check_VarExpr(out, as->leftVal->pos, name);
            assign_type_check(out, left_type, as->rightVal);
            if(cur_token2ArrLen.find(name) != cur_token2ArrLen.end()) {
                error_print(out, as->leftVal->pos, "Array can't be assigned");
            }
            if(cur_token2Type.find(name) == cur_token2Type.end() && g_token2ArrLen.find(name) != g_token2ArrLen.end()) {
                error_print(out, as->leftVal->pos, "Array can't be assigned");
            }
        }
            break;
        case A_leftValType::A_arrValKind:{
            /* write your code here */
            left_type = check_ArrayExpr(out, as->leftVal->u.arrExpr);
            assign_type_check(out, left_type, as->rightVal);
        }
            break;
        case A_leftValType::A_memberValKind:{
            /* write your code here */
            left_type = check_MemberExpr(out, as->leftVal->u.memberExpr);
            assign_type_check(out, left_type, as->rightVal);
        }
            break;
    }
    return;
}

aA_type check_VarExpr(std::ostream* out, A_pos pos, string name) {
    aA_type ret = nullptr;
    if (func2Param.find(name) != func2Param.end()) {
        error_print(out, pos, "Func name can't be used as a var");
    }
    if (struct2Members.find(name) != struct2Members.end()) {
        error_print(out, pos, "Struct name can't be used as a var");
    }
    if (cur_token2Type.find(name) != cur_token2Type.end()) { // local or param
        ret = cur_token2Type.at(name);
    }
    else {
        if (g_token2Type.find(name) != g_token2Type.end()) {
            ret = g_token2Type.at(name);
        }
        else {
            error_print(out, pos, "Var " + name + " not declared");
        }
    }
    return ret;
}

aA_type check_ArrayExpr(std::ostream* out, aA_arrayExpr ae){
    if(!ae)
        return nullptr;
    /*
        Example:
            a[1] = 0;
        Hint:
            check the validity of the array index
    */
    aA_type ret = nullptr;
    string name = *(ae->arr);

    ret = check_VarExpr(out, ae->pos, name);

    if (cur_token2ArrLen.find(name) == cur_token2ArrLen.end() && g_token2ArrLen.find(name) == g_token2ArrLen.end()) {
        error_print(out, ae->pos, name + " is not array!");
    }

    if(ae->idx->kind == A_indexExprKind::A_idIndexKind) {
        string id = *(ae->idx->u.id);
        aA_type type;
        type = check_VarExpr(out, ae->idx->pos, id);

        if(get_type(type) != "int") {
            error_print(out, ae->idx->pos, "Id " + id + " type in array must be int");
        }
    }
    if(ae->idx->kind == A_indexExprKind::A_numIndexKind) {
        if(ae->idx->u.num < 0) {
            error_print(out, ae->idx->pos, "Num in array must > 0");
        }
    }
    return ret;
}


aA_type check_MemberExpr(std::ostream* out, aA_memberExpr me){
    // check if the member exists and return the tyep of the member
    // you may need to check if the type of this expression matches with its 
    // leftvalue or rightvalue, so return its aA_type would be a good way. Feel 
    // free to change the design pattern if you need.
    if(!me)
        return nullptr;
    /*
        Example:
            a.b
    */
    aA_type ret;
    string name = *(me->structId);
    string member = *(me->memberId);

    aA_type structT = check_VarExpr(out, me->pos, name);
    if(structT->type == A_dataType::A_nativeTypeKind) {
        error_print(out, me->pos, name + " is not a struct var");
    }
    string structName = *(structT->u.structType);

    if (struct2Members.find(structName) == struct2Members.end()) { 
        error_print(out, me->pos, "Struct name " + structName + " not exist");
    }
    vector<aA_varDecl> *v = struct2Members.at(structName);
    // vector<aA_varDecl> *v = nullptr;
    if ((*v).size() == 0) {
        error_print(out, me->pos, "Struct member " + member + " not exist");
    }
    string cur_id;
    for (auto var : *v) {
        if (var->kind == A_varDeclType::A_varDeclScalarKind) {
            aA_varDeclScalar vds = var->u.declScalar;
            cur_id = *(vds->id);
            if (cur_id == member) {
                return vds->type;
            }
        }
        if (var->kind == A_varDeclType::A_varDeclArrayKind) {
            aA_varDeclArray vda = var->u.declArray;
            cur_id = *(vda->id);
            if (cur_id == member) {
                return vda->type;
            }
        }
    }
    // no struct member match
    error_print(out, me->pos, "Struct member " + member + " not exist");

    return nullptr;
}


void check_IfStmt(std::ostream* out, aA_ifStmt is){
    if(!is)
        return;
    check_BoolExpr(out, is->boolExpr);
    check_CodeblockStmtList(out, is->ifStmts);
    check_CodeblockStmtList(out, is->elseStmts);
    // for(aA_codeBlockStmt s : is->ifStmts){
    //     check_CodeblockStmt(out, s);
    // }
    // for(aA_codeBlockStmt s : is->elseStmts){
    //     check_CodeblockStmt(out, s);
    // }
    return;
}


void check_BoolExpr(std::ostream* out, aA_boolExpr be){
    if(!be)
        return;
    switch (be->kind)
    {
    case A_boolExprType::A_boolBiOpExprKind:
        /* write your code here */
        check_BoolExpr(out, be->u.boolBiOpExpr->left);
        check_BoolExpr(out, be->u.boolBiOpExpr->right);
        break;
    case A_boolExprType::A_boolUnitKind:
        check_BoolUnit(out, be->u.boolUnit);
        break;
    default:
        break;
    }
    return;
}


void check_BoolUnit(std::ostream* out, aA_boolUnit bu){
    if(!bu)
        return;
    switch (bu->kind)
    {
        case A_boolUnitType::A_comOpExprKind:{
            /* write your code here */
            aA_exprUnit left = bu->u.comExpr->left;
            aA_type type = check_ExprUnit(out, left);
            if(get_type(type) != "int") {
                error_print(out, left->pos, "Variable type in comparison must be int");
            }
            bool isArr = check_ExprUnitArr(left);
            if(isArr) {
                error_print(out, left->pos, "Variable in bool expression must not be entire array");
            }
            // check_ExprUnitArr(bu->u.comExpr->left);
            aA_exprUnit right = bu->u.comExpr->right;
            type = check_ExprUnit(out, right);
            if(get_type(type) != "int") {
                error_print(out, right->pos, "Variable type in comparison must be int");
            }
            isArr = check_ExprUnitArr(right);
            if(isArr) {
                error_print(out, right->pos, "Variable in bool expression must not be entire array");
            }
            
            break;
        }
            
        case A_boolUnitType::A_boolExprKind: {
            /* write your code here */
            check_BoolExpr(out, bu->u.boolExpr);
            break;
        }

        case A_boolUnitType::A_boolUOpExprKind: {
            /* write your code here */
            check_BoolUnit(out, bu->u.boolUOpExpr->cond);
            break;
        }

        default:
            break;
    }
    return;
}
bool check_ExprUnitArr(aA_exprUnit eu){
    if(!eu)
        return false;
    switch (eu->kind)
    {
        case A_exprUnitType::A_idExprKind:{
            /* write your code here */
            string id = *(eu->u.id);
            if(cur_token2ArrLen.find(id) != cur_token2ArrLen.end()) {
                return true;
            }
            else {
                if(g_token2ArrLen.find(id) != g_token2ArrLen.end() && cur_token2Type.find(id) == cur_token2Type.end()) {
                    return true;
                }
            }
        }
            break;
        // case A_exprUnitType::A_memberExprKind:{
        //     /* write your code here */
        //     ret = check_MemberExpr(out, eu->u.memberExpr);
        // }
            break;
        case A_exprUnitType::A_arithExprKind:{
            /* write your code here */
            if (eu->u.arithExpr->kind == A_arithExprType::A_exprUnitKind) {
                return check_ExprUnitArr(eu->u.arithExpr->u.exprUnit);
            }
        }
            break;
        case A_exprUnitType::A_arithUExprKind:{
            /* write your code here */
            return check_ExprUnitArr(eu->u.arithUExpr->expr);
        }
            break;
    }
    return false;
}

aA_type check_ExprUnit(std::ostream* out, aA_exprUnit eu){
    // validate the expression unit and return the aA_type of it
    // you may need to check if the type of this expression matches with its 
    // leftvalue or rightvalue, so return its aA_type would be a good way. 
    // Feel free to change the design pattern if you need.
    if(!eu)
        return nullptr;
    aA_type ret;
    switch (eu->kind)
    {
        case A_exprUnitType::A_idExprKind:{
            /* write your code here */
            string id = *(eu->u.id);
            ret = check_VarExpr(out, eu->pos, id);
        }
            break;
        case A_exprUnitType::A_numExprKind:{
            /* write your code here */
            ret = aA_Type(A_NativeType(eu->pos, A_nativeType::A_intTypeKind));
        }
            break;
        case A_exprUnitType::A_fnCallKind:{
            /* write your code here */
            ret = check_FuncCall(out, eu->u.callExpr); 
        }
            break;
        case A_exprUnitType::A_arrayExprKind:{
            /* write your code here */
            ret = check_ArrayExpr(out, eu->u.arrayExpr);
        }
            break;
        case A_exprUnitType::A_memberExprKind:{
            /* write your code here */
            ret = check_MemberExpr(out, eu->u.memberExpr);
        }
            break;
        case A_exprUnitType::A_arithExprKind:{
            /* write your code here */
            if (eu->u.arithExpr->kind == A_arithExprType::A_arithBiOpExprKind) {
                check_ArithBiOpExpr(out, eu->u.arithExpr->u.arithBiOpExpr);
                ret = aA_Type(A_NativeType(eu->pos, A_nativeType::A_intTypeKind));
            }
            if (eu->u.arithExpr->kind == A_arithExprType::A_exprUnitKind) {
                ret = check_ExprUnit(out, eu->u.arithExpr->u.exprUnit);
            }
        }
            break;
        case A_exprUnitType::A_arithUExprKind:{
            /* write your code here */
            ret = check_ExprUnit(out, eu->u.arithUExpr->expr);
        }
            break;
    }
    return ret;
}


aA_type check_FuncCall(std::ostream* out, aA_fnCall fc){
    if(!fc)
        return nullptr;
    // Example:
    //      foo(1, 2);

    /* write your code here */
    aA_type ret = nullptr;
    aA_type cur;
    string name = *(fc->fn);

    if (func2Param.find(name) == func2Param.end()) {
        error_print(out, fc->pos, "Function name " + name + " not exist");
    }
    else {
        // if ((funcStat.at(name) & 2) == 0) { //not defined
        //     error_print(out, fc->pos, "Function " + name + " not defined");
        // }
        ret = g_token2Type.at(name);
    }
    
    vector<aA_rightVal> vals = fc->vals;
    vector<aA_varDecl> vars = *(func2Param.at(name));
  
    if(vals.size() != vars.size()) {
        error_print(out, fc->pos, "Param number incompatible with function " + name);
    }
    for(int i = 0; i < vals.size(); i++) {
        
        if (vars[i]->kind == A_varDeclType::A_varDeclScalarKind) {
            cur = vars[i]->u.declScalar->type;
            assign_arr_check(out, false, vals[i]);
        }
        if (vars[i]->kind == A_varDeclType::A_varDeclArrayKind) {
            cur = vars[i]->u.declArray->type;
            assign_arr_check(out, true, vals[i]);
        }
        assign_type_check(out, cur, vals[i]);
    
    }

    return ret;
}


void check_WhileStmt(std::ostream* out, aA_whileStmt ws){
    if(!ws)
        return;
    check_BoolExpr(out, ws->boolExpr);
    check_CodeblockStmtList(out, ws->whileStmts);
    // for(aA_codeBlockStmt s : ws->whileStmts){
    //     check_CodeblockStmt(out, s);
    // }
    return;
}


void check_CallStmt(std::ostream* out, aA_callStmt cs){
    if(!cs)
        return;
    check_FuncCall(out, cs->fnCall);
    return;
}


void check_ReturnStmt(std::ostream* out, aA_returnStmt rs){
    if(!rs)
        return;

    aA_type type = g_token2Type.at(cur_fn);
    assign_type_check(out, type, rs->retVal);
    return;
}

void check_ArithBiOpExpr(std::ostream* out, aA_arithBiOpExpr ae) {
    aA_arithExpr left = ae->left;
    if(left->kind == A_arithExprType::A_arithBiOpExprKind) {
        check_ArithBiOpExpr(out, left->u.arithBiOpExpr);
    }
    if(left->kind == A_arithExprType::A_exprUnitKind) {
        aA_type type = check_ExprUnit(out, left->u.exprUnit);
        if(get_type(type) != "int") {
            error_print(out, left->u.exprUnit->pos, "Variable type in arithmetic must be int");
        }
        bool isArr = check_ExprUnitArr(left->u.exprUnit);
        if(isArr) {
            error_print(out, left->u.exprUnit->pos, "Variable in arithmetic must not be entire array");
        }
    }

    aA_arithExpr right = ae->right;
    if(right->kind == A_arithExprType::A_arithBiOpExprKind) {
        check_ArithBiOpExpr(out, right->u.arithBiOpExpr);
    }
    if(right->kind == A_arithExprType::A_exprUnitKind) {
        aA_type type = check_ExprUnit(out, right->u.exprUnit);
        if(get_type(type) != "int") {
            error_print(out, right->u.exprUnit->pos, "Variable type in arithmetic must be int");
        }
        bool isArr = check_ExprUnitArr(right->u.exprUnit);
        if(isArr) {
            error_print(out, right->u.exprUnit->pos, "Variable in arithmetic must not be entire array");
        }
    }
}