#include "liveness.h"
#include <unordered_map>
#include <unordered_set>
#include "graph.hpp"
#include "llvm_ir.h"
#include "temp.h"

using namespace std;
using namespace LLVMIR;
using namespace GRAPH;

struct inOut {
    TempSet_ in;
    TempSet_ out;
};

struct useDef {
    TempSet_ use;
    TempSet_ def;
};

static unordered_map<GRAPH::Node<LLVMIR::L_block*>*, inOut> InOutTable;
static unordered_map<GRAPH::Node<LLVMIR::L_block*>*, useDef> UseDefTable;

list<AS_operand**> get_all_AS_operand(L_stm* stm) {
    list<AS_operand**> AS_operand_list;
    switch (stm->type) {
        case L_StmKind::T_BINOP: {
            AS_operand_list.push_back(&(stm->u.BINOP->left));
            AS_operand_list.push_back(&(stm->u.BINOP->right));
            AS_operand_list.push_back(&(stm->u.BINOP->dst));

        } break;
        case L_StmKind::T_LOAD: {
            AS_operand_list.push_back(&(stm->u.LOAD->dst));
            AS_operand_list.push_back(&(stm->u.LOAD->ptr));
        } break;
        case L_StmKind::T_STORE: {
            AS_operand_list.push_back(&(stm->u.STORE->src));
            AS_operand_list.push_back(&(stm->u.STORE->ptr));
        } break;
        case L_StmKind::T_LABEL: {
        } break;
        case L_StmKind::T_JUMP: {
        } break;
        case L_StmKind::T_CMP: {
            AS_operand_list.push_back(&(stm->u.CMP->left));
            AS_operand_list.push_back(&(stm->u.CMP->right));
            AS_operand_list.push_back(&(stm->u.CMP->dst));
        } break;
        case L_StmKind::T_CJUMP: {
            AS_operand_list.push_back(&(stm->u.CJUMP->dst));
        } break;
        case L_StmKind::T_MOVE: {
            AS_operand_list.push_back(&(stm->u.MOVE->src));
            AS_operand_list.push_back(&(stm->u.MOVE->dst));
        } break;
        case L_StmKind::T_CALL: {
            AS_operand_list.push_back(&(stm->u.CALL->res));
            for (auto& arg : stm->u.CALL->args) {
                AS_operand_list.push_back(&arg);
            }
        } break;
        case L_StmKind::T_VOID_CALL: {
            for (auto& arg : stm->u.VOID_CALL->args) {
                AS_operand_list.push_back(&arg);
            }
        } break;
        case L_StmKind::T_RETURN: {
            if (stm->u.RET->ret != nullptr)
                AS_operand_list.push_back(&(stm->u.RET->ret));
        } break;
        case L_StmKind::T_PHI: {
            AS_operand_list.push_back(&(stm->u.PHI->dst));
            for (auto& phi : stm->u.PHI->phis) {
                AS_operand_list.push_back(&(phi.first));
            }
        } break;
        case L_StmKind::T_ALLOCA: {
            AS_operand_list.push_back(&(stm->u.ALLOCA->dst));
        } break;
        case L_StmKind::T_GEP: {
            AS_operand_list.push_back(&(stm->u.GEP->new_ptr));
            AS_operand_list.push_back(&(stm->u.GEP->base_ptr));
            AS_operand_list.push_back(&(stm->u.GEP->index));
        } break;
        default: {
            printf("%d\n", (int)stm->type);
            assert(0);
        }
    }
    return AS_operand_list;
}

std::list<AS_operand**> get_def_operand(L_stm* stm) {
    //   Todo
    list<AS_operand**> AS_operand_list;
    switch (stm->type) {
        case L_StmKind::T_BINOP: {
            if(stm->u.BINOP->dst->kind == OperandKind::TEMP)  AS_operand_list.push_back(&(stm->u.BINOP->dst));
        } break;
        case L_StmKind::T_LOAD: {
            if(stm->u.LOAD->dst->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.LOAD->dst));
        } break;
        case L_StmKind::T_STORE: {
            if(stm->u.STORE->ptr->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.STORE->ptr));
        } break;
        case L_StmKind::T_LABEL: {
        } break;
        case L_StmKind::T_JUMP: {
        } break;
        case L_StmKind::T_CMP: {
            if(stm->u.CMP->dst->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.CMP->dst));
        } break;
        case L_StmKind::T_CJUMP: {
        } break;
        case L_StmKind::T_MOVE: {
            if(stm->u.MOVE->dst->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.MOVE->dst));
        } break;
        case L_StmKind::T_CALL: {
            if(stm->u.CALL->res->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.CALL->res));
        } break;
        case L_StmKind::T_VOID_CALL: {
        } break;
        case L_StmKind::T_RETURN: {
        } break;
        case L_StmKind::T_PHI: {
            if(stm->u.PHI->dst->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.PHI->dst));
        } break;
        case L_StmKind::T_ALLOCA: {
            if(stm->u.ALLOCA->dst->kind == OperandKind::TEMP)  AS_operand_list.push_back(&(stm->u.ALLOCA->dst));
        } break;
        case L_StmKind::T_GEP: {
            AS_operand_list.push_back(&(stm->u.GEP->new_ptr));
        } break;
        default: {
            printf("%d\n", (int)stm->type);
            assert(0);
        }
    }
    return AS_operand_list;
}
list<Temp_temp*> get_def(L_stm* stm) {
    auto AS_operand_list = get_def_operand(stm);
    list<Temp_temp*> Temp_list;
    for (auto AS_op : AS_operand_list) {
        Temp_list.push_back((*AS_op)->u.TEMP);
    }
    return Temp_list;
}

std::list<AS_operand**> get_use_operand(L_stm* stm) {
    //   Todo
    list<AS_operand**> AS_operand_list;
    switch (stm->type) {
        case L_StmKind::T_BINOP: {
            if(stm->u.BINOP->left->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.BINOP->left));
            if(stm->u.BINOP->right->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.BINOP->right));
        } break;
        case L_StmKind::T_LOAD: {
            if(stm->u.LOAD->ptr->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.LOAD->ptr));
        } break;
        case L_StmKind::T_STORE: {
           if(stm->u.STORE->src->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.STORE->src));
        } break;
        case L_StmKind::T_LABEL: {
        } break;
        case L_StmKind::T_JUMP: {
        } break;
        case L_StmKind::T_CMP: {
            if(stm->u.CMP->left->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.CMP->left));
            if(stm->u.CMP->right->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.CMP->right));
        } break;
        case L_StmKind::T_CJUMP: {
            if(stm->u.CJUMP->dst->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.CJUMP->dst));
        } break;
        case L_StmKind::T_MOVE: {
            if(stm->u.MOVE->src->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.MOVE->src));
        } break;
        case L_StmKind::T_CALL: {
            for (auto& arg : stm->u.CALL->args) {
                if(arg->kind == OperandKind::TEMP) AS_operand_list.push_back(&arg);
            }
        } break;
        case L_StmKind::T_VOID_CALL: {
            for (auto& arg : stm->u.VOID_CALL->args) {
                if(arg->kind == OperandKind::TEMP) AS_operand_list.push_back(&arg);
            }
        } break;
        case L_StmKind::T_RETURN: {
            if (stm->u.RET->ret != nullptr)
                if(stm->u.RET->ret->kind == OperandKind::TEMP) 
                    AS_operand_list.push_back(&(stm->u.RET->ret));
        } break;
        case L_StmKind::T_PHI: { // should not enter here
            for (auto& phi : stm->u.PHI->phis) {
                if(phi.first->kind == OperandKind::TEMP)  AS_operand_list.push_back(&(phi.first));
            }
        } break;
        case L_StmKind::T_ALLOCA: {
        } break;
        case L_StmKind::T_GEP: {
            if(stm->u.GEP->base_ptr->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.GEP->base_ptr));
            if(stm->u.GEP->index->kind == OperandKind::TEMP) AS_operand_list.push_back(&(stm->u.GEP->index));
        } break;
        default: {
            printf("%d\n", (int)stm->type);
            assert(0);
        }
    }
    return AS_operand_list;
}
list<Temp_temp*> get_use(L_stm* stm) {
    auto AS_operand_list = get_use_operand(stm);
    list<Temp_temp*> Temp_list;
    for (auto AS_op : AS_operand_list) {
        Temp_list.push_back((*AS_op)->u.TEMP);
    }
    return Temp_list;
}

static void init_INOUT() {
    InOutTable.clear();
    UseDefTable.clear();
}

TempSet_& FG_Out(GRAPH::Node<LLVMIR::L_block*>* r) {
    return InOutTable[r].out;
}
TempSet_& FG_In(GRAPH::Node<LLVMIR::L_block*>* r) {
    return InOutTable[r].in;
}
TempSet_& FG_def(GRAPH::Node<LLVMIR::L_block*>* r) {
    return UseDefTable[r].def;
}
TempSet_& FG_use(GRAPH::Node<LLVMIR::L_block*>* r) {
    return UseDefTable[r].use;
}

static void Use_def(GRAPH::Node<LLVMIR::L_block*>* r, GRAPH::Graph<LLVMIR::L_block*>& bg, std::vector<Temp_temp*>& args) {
//    Todo
    unordered_set<Temp_temp*> defined;
    unordered_set<Temp_temp*> args_set;
    for(auto temp:args) args_set.insert(temp);
    for(auto block_node:bg.mynodes){
        defined.clear();
        args_set.clear();
        auto block=block_node.second->info;
        for(L_stm* stm:block->instrs){
            auto def_list=get_def(stm);
            auto use_list=get_use(stm);
            for(auto temp:use_list){
                if(defined.find(temp)==defined.end() && args_set.find(temp)==args_set.end())
                    FG_use(block_node.second).insert(temp);
            }
            for(auto temp:def_list){
                FG_def(block_node.second).insert(temp);
                defined.insert(temp);
            }

        }
    }
}
static int gi=0;
static bool LivenessIteration(GRAPH::Node<LLVMIR::L_block*>* r, GRAPH::Graph<LLVMIR::L_block*>& bg) {
   //    Todo
    bool changed = false;
    if(gi == 0) {
        for(auto block_node:bg.mynodes){
            FG_In(block_node.second)=FG_use(block_node.second);
            // FG_Out(block_node.second);
        }
    }
    gi++;
    for(auto block_node:bg.mynodes){
        auto block=block_node.second->info;
        TempSet_ In=FG_In(block_node.second);
        TempSet_ Out=FG_Out(block_node.second);
        TempSet_ def=FG_def(block_node.second);
        TempSet_ use=FG_use(block_node.second);
        TempSet diff = TempSet_diff(&Out, &def);
        // TempSet_ new_in = *TempSet_diff(&use, TempSet_diff(TempSet_union(&use, diff), diff)); // intersect
        TempSet_ new_in = *TempSet_union(&use, diff);
        TempSet_ new_out = Out;
        for(auto node : block_node.second->succs){
            new_out = *TempSet_union(&new_out, &FG_In(bg.mynodes[node]));
        }
        if(!TempSet_eq(&new_in, &In) || !TempSet_eq(&new_out, &Out)){
            FG_In(block_node.second)=new_in;
            FG_Out(block_node.second)=new_out;
            changed = true;
        }
        
    }
    return changed;
}

void PrintTemps(FILE *out, TempSet set) {
    for(auto x:*set){
        fprintf(out, "%d  ",x->num);
    }
}


void Show_Liveness(FILE* out, GRAPH::Graph<LLVMIR::L_block*>& bg) {
    fprintf(out, "\n\nNumber of iterations=%d\n\n", gi);
    for(auto block_node:bg.mynodes){
        fprintf(out, "----------------------\n");
        printf("block %s \n",block_node.second->info->label->name.c_str());
        fprintf(out, "def=\n"); PrintTemps(out, &FG_def(block_node.second)); fprintf(out, "\n");
        fprintf(out, "use=\n"); PrintTemps(out, &FG_use(block_node.second)); fprintf(out, "\n");
        fprintf(out, "In=\n");  PrintTemps(out, &FG_In(block_node.second)); fprintf(out, "\n");
        fprintf(out, "Out=\n"); PrintTemps(out, &FG_Out(block_node.second)); fprintf(out, "\n");
    }
}
// 以block为单位
void Liveness(GRAPH::Node<LLVMIR::L_block*>* r, GRAPH::Graph<LLVMIR::L_block*>& bg, std::vector<Temp_temp*>& args) {
    init_INOUT();
    Use_def(r, bg, args);
    gi=0;
    bool changed = true;
    while (changed)
        changed = LivenessIteration(r, bg);
    Show_Liveness(stderr, bg);
}
