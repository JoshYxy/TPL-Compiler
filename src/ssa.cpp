#include "ssa.h"
#include <cassert>
#include <iostream>
#include <list>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "bg_llvm.h"
#include "graph.hpp"
#include "liveness.h"
#include "printLLVM.h"

#include <algorithm>

using namespace std;
using namespace LLVMIR;
using namespace GRAPH;
struct imm_Dominator {
    LLVMIR::L_block* pred;
    unordered_set<LLVMIR::L_block*> succs;
};

unordered_map<L_block*, unordered_set<L_block*>> dominators;
unordered_map<L_block*, imm_Dominator> tree_dominators;
unordered_map<L_block*, unordered_set<L_block*>> DF_array;
unordered_map<L_block*, Node<LLVMIR::L_block*>*> revers_graph;
unordered_map<Temp_temp*, AS_operand*> temp2ASoper;

static void init_table() {
    dominators.clear();
    tree_dominators.clear();
    DF_array.clear();
    revers_graph.clear();
    temp2ASoper.clear();
}

LLVMIR::L_prog* SSA(LLVMIR::L_prog* prog) {
    for (auto& fun : prog->funcs) {
        init_table();
        combine_addr(fun);
        mem2reg(fun);
        auto RA_bg = Create_bg(fun->blocks);
        SingleSourceGraph(RA_bg.mynodes[0], RA_bg,fun);
        // Show_graph(stdout,RA_bg);
        Liveness(RA_bg.mynodes[0], RA_bg, fun->args);
        Dominators(RA_bg);
        // printf_domi();
        tree_Dominators(RA_bg);
        // printf_D_tree();
        // 默认0是入口block
        computeDF(RA_bg, RA_bg.mynodes[0]);
        // printf_DF();
        Place_phi_fu(RA_bg, fun);
        Rename(RA_bg);
        combine_addr(fun);
    }
    return prog;
}

static bool is_mem_variable(L_stm* stm) {
    return stm->type == L_StmKind::T_ALLOCA && stm->u.ALLOCA->dst->kind == OperandKind::TEMP && stm->u.ALLOCA->dst->u.TEMP->type == TempType::INT_PTR && stm->u.ALLOCA->dst->u.TEMP->len == 0;
}

static list<AS_operand**> get_def_int_operand(LLVMIR::L_stm* stm) {
    list<AS_operand**> ret1 = get_def_operand(stm), ret2;
    for (auto AS_op : ret1) {
        if ((**AS_op).u.TEMP->type == TempType::INT_TEMP) {
            ret2.push_back(AS_op);
        }
    }
    return ret2;
}

static list<AS_operand**> get_use_int_operand(LLVMIR::L_stm* stm) {
    list<AS_operand**> ret1 = get_use_operand(stm), ret2;
    for (auto AS_op : ret1) {
        if ((**AS_op).u.TEMP->type == TempType::INT_TEMP) {
            ret2.push_back(AS_op);
        }
    }
    return ret2;
}
// 保证相同的AS_operand,地址一样 。常量除外
void combine_addr(LLVMIR::L_func* fun) {
    unordered_map<Temp_temp*, unordered_set<AS_operand**>> temp_set;
    unordered_map<Name_name*, unordered_set<AS_operand**>> name_set;
    for (auto& block : fun->blocks) {
        for (auto& stm : block->instrs) {
            auto AS_operand_list = get_all_AS_operand(stm);
            for (auto AS_op : AS_operand_list) {
                if ((*AS_op)->kind == OperandKind::TEMP) {
                    temp_set[(*AS_op)->u.TEMP].insert(AS_op);
                } else if ((*AS_op)->kind == OperandKind::NAME) {
                    name_set[(*AS_op)->u.NAME].insert(AS_op);
                }
            }
        }
    }
    for (auto temp : temp_set) {
        AS_operand* fi_AS_op = **temp.second.begin();
        for (auto AS_op : temp.second) {
            *AS_op = fi_AS_op;
        }
    }
    for (auto name : name_set) {
        AS_operand* fi_AS_op = **name.second.begin();
        for (auto AS_op : name.second) {
            *AS_op = fi_AS_op;
        }
    }
}

void mem2reg(LLVMIR::L_func* fun) {
   //   Todo
    for(auto block : fun->blocks) {
        for(auto it = block->instrs.begin(); it != block->instrs.end(); it++) { 
            auto stm = *it;
            if(is_mem_variable(stm)) {
                Temp_temp* old = stm->u.ALLOCA->dst->u.TEMP;
                AS_operand* temp = AS_Operand_Temp(Temp_newtemp_int());
                temp2ASoper[old] = temp;
                L_stm* new_stm = L_Move(AS_Operand_Const(0), temp);
                it = block->instrs.erase(it);
                block->instrs.insert(it, new_stm);
                it--;
            }
            if(stm->type == L_StmKind::T_STORE) {
                if(stm->u.STORE->ptr->kind == OperandKind::TEMP 
                && temp2ASoper.find(stm->u.STORE->ptr->u.TEMP) != temp2ASoper.end()) {
                    AS_operand* src = stm->u.STORE->src;
                    if(src->kind == OperandKind::TEMP) {
                        bool found = false;
                        for(auto it2 = block->instrs.begin(); it2 != block->instrs.end(); it2++) { // search for use of the load AS
                            auto pre_stm = *it2; 
                            list<AS_operand**> AS_list = get_def_int_operand(pre_stm);
                            for(auto as : AS_list) {
                                if(*as == src) {
                                    *as = temp2ASoper[stm->u.STORE->ptr->u.TEMP];
                                    found = true;
                                }
                            }
                            if(found) break;
                        }
                        if(found) {
                            it = block->instrs.erase(it);
                            it--;
                        }
                        else {
                            L_stm* new_stm = L_Move(src, temp2ASoper[stm->u.STORE->ptr->u.TEMP]);
                            it = block->instrs.erase(it);
                            block->instrs.insert(it, new_stm);
                            it--; 
                        }
                    }
                    else {
                        L_stm* new_stm = L_Move(src, temp2ASoper[stm->u.STORE->ptr->u.TEMP]);
                        it = block->instrs.erase(it);
                        block->instrs.insert(it, new_stm);
                        it--;                        
                    }

                }
            }
            if(stm->type == L_StmKind::T_LOAD) {
                if(stm->u.LOAD->ptr->kind == OperandKind::TEMP 
                && temp2ASoper.find(stm->u.LOAD->ptr->u.TEMP) != temp2ASoper.end()) {
                    AS_operand* temp = stm->u.LOAD->dst;
                    AS_operand* new_temp = temp2ASoper[stm->u.LOAD->ptr->u.TEMP];
                    it = block->instrs.erase(it);        
                    it--;
                    for(auto it2 = block->instrs.begin(); it2 != block->instrs.end(); it2++) { // search for use of the load AS
                        auto next_stm = *it2;
                        list<AS_operand**> AS_list = get_use_int_operand(next_stm);
                        for(auto as : AS_list) {
                            if(*as == temp) {
                                *as = new_temp;
                            }
                        }
                    //     switch (next_stm->type) {
                    //         case L_StmKind::T_BINOP: {
                    //             if(next_stm->u.BINOP->left == temp) next_stm->u.BINOP->left = new_temp;
                    //             if(next_stm->u.BINOP->right == temp) next_stm->u.BINOP->right = new_temp;
                    //         } break;
                    //         case L_StmKind::T_LOAD: {
                    //         } break;
                    //         case L_StmKind::T_STORE: {
                    //             if(next_stm->u.STORE->src == temp) next_stm->u.STORE->src = new_temp;
                    //         } break;
                    //         case L_StmKind::T_LABEL: {
                    //         } break;
                    //         case L_StmKind::T_JUMP: {
                    //         } break;
                    //         case L_StmKind::T_CMP: {
                    //             if(next_stm->u.CMP->left == temp) next_stm->u.CMP->left = new_temp;
                    //             if(next_stm->u.CMP->right == temp) next_stm->u.CMP->right = new_temp;
                    //         } break;
                    //         case L_StmKind::T_CJUMP: {
                    //             if(next_stm->u.CJUMP->dst == temp) next_stm->u.CJUMP->dst = new_temp;
                    //         } break;
                    //         case L_StmKind::T_MOVE: {
                    //             if(next_stm->u.MOVE->src == temp) next_stm->u.MOVE->src = new_temp;
                    //         } break;
                    //         case L_StmKind::T_CALL: {
                    //             for (auto& arg : next_stm->u.CALL->args) {
                    //                 if(arg == temp) arg = new_temp;
                    //             }
                    //         } break;
                    //         case L_StmKind::T_VOID_CALL: {
                    //             for (auto& arg : next_stm->u.VOID_CALL->args) {
                    //                 if(arg == temp) arg = new_temp;
                    //             }
                    //         } break;
                    //         case L_StmKind::T_RETURN: {
                    //             if(next_stm->u.RET->ret == temp) next_stm->u.RET->ret = new_temp;
                    //         } break;
                    //         case L_StmKind::T_PHI: {
                    //         } break;
                    //         case L_StmKind::T_ALLOCA: {
                    //         } break;
                    //         case L_StmKind::T_GEP: {
                    //             if(next_stm->u.GEP->index == temp) next_stm->u.GEP->index = new_temp;
                    //         } break;
                    //         default: {
                    //             printf("%d\n", (int)stm->type);
                    //             assert(0);
                    //         }
                    //     }
                    }
                }
            }
        }
    }
}

void Dominators(GRAPH::Graph<LLVMIR::L_block*>& bg) {
    //   Todo
    //set revers_graph 
    for(auto node : bg.mynodes) {
        revers_graph[node.second->info] = node.second;
    }
    dominators[bg.mynodes[0]->info].insert(bg.mynodes[0]->info);
    for (auto node : bg.mynodes) {
        if (node.first == 0) continue;
        if (node.first == 1) {
            dominators[node.second->info] = unordered_set<L_block*>();
            for (auto node2 : bg.mynodes) {
                dominators[node.second->info].insert(node2.second->info);
            }
            continue;
        }
        dominators[node.second->info] = dominators[bg.mynodes[1]->info];
    }
    bool changed = true;
 
    while(changed) {
        changed = false;
        for(auto node : bg.mynodes) {
            if(node.first == 0) continue;
       
            unordered_set<L_block*> temp = dominators[node.second->info];
            for(auto p : node.second->preds) {
                unordered_set<L_block*> swp = temp;
                for(auto ele : temp) {
                    if(dominators[bg.mynodes[p]->info].find(ele) == dominators[bg.mynodes[p]->info].end()) { // intersection
                        swp.erase(ele);
                    }
                }
                temp = swp;
            }
            temp.insert(node.second->info);
            // cout<<node.second->info->label->name<<temp.size()<<endl;
            if(temp != dominators[node.second->info]) {
                changed = true;
                dominators[node.second->info] = temp;
            }
        }
    }
}

void printf_domi() {
    printf("Dominator:\n");
    for (auto x : dominators) {
        printf("%s :\n", x.first->label->name.c_str());
        for (auto t : x.second) {
            printf("%s ", t->label->name.c_str());
        }
        printf("\n\n");
    }
}

void printf_D_tree() {
    printf("dominator tree:\n");
    for (auto x : tree_dominators) {
        printf("%s :\n", x.first->label->name.c_str());
        for (auto t : x.second.succs) {
            printf("%s ", t->label->name.c_str());
        }
        printf("\n\n");
    }
}
void printf_DF() {
    printf("DF:\n");
    for (auto x : DF_array) {
        printf("%s :\n", x.first->label->name.c_str());
        for (auto t : x.second) {
            printf("%s ", t->label->name.c_str());
        }
        printf("\n\n");
    }
}

void tree_Dominators(GRAPH::Graph<LLVMIR::L_block*>& bg) {
    //   Todo
    for(auto node:bg.mynodes) {
        if(node.first == 0) continue;
    
        for(L_block* dom : dominators[node.second->info]) {
            if(dom == node.second->info) continue;
            bool imm = true;
            for(auto dom2 : dominators[node.second->info]) { // dominators of dominator 'dom'
                if(dom2 == node.second->info || dom2 == dom) continue; 
                if(dominators[dom2].find(dom) != dominators[dom2].end()) {
                    imm = false;
                    break;
                }
            }
            if(imm) {
                tree_dominators[node.second->info].pred = dom;
                tree_dominators[dom].succs.insert(node.second->info);
                break;
            }
        }
    
    }
}

void computeDF(GRAPH::Graph<LLVMIR::L_block*>& bg, GRAPH::Node<LLVMIR::L_block*>* r) {
    //   Todo
    auto node = r;
    for(auto succ : node->succs) {
        if(tree_dominators[bg.mynodes[succ]->info].pred != node->info) {
            DF_array[node->info].insert(bg.mynodes[succ]->info);
        }
    }
    for(auto child : tree_dominators[node->info].succs) {
        computeDF(bg, revers_graph[child]);
        for(auto ele : DF_array[child]) {
            if(dominators[ele].find(node->info) == dominators[ele].end() || ele == node->info) {
                DF_array[node->info].insert(ele);
            }
        }
    }

    
}

// 只对标量做
void Place_phi_fu(GRAPH::Graph<LLVMIR::L_block*>& bg, L_func* fun) {
    //   Todo
    unordered_map<Temp_temp*, unordered_set<L_block*>> temp2block; // defsites
    unordered_map<L_block*, unordered_set<Temp_temp*>> phi2temp; // A phi
    for(auto node : bg.mynodes) {
        for(auto temp : FG_def(node.second)) {
            if(temp->type == TempType::INT_TEMP) {
                temp2block[temp].insert(node.second->info);
            }
        }
    }
    for(auto temp : temp2block) {
        while(!temp.second.empty()) {
            L_block* block = *temp.second.begin();
            temp.second.erase(temp.second.begin());
            for(auto block2 : DF_array[block]) { // block2:Y
                auto node = revers_graph[block2];
                if(FG_In(node).find(temp.first) == FG_In(node).end()) {
                    continue;
                }
                if(phi2temp[block2].find(temp.first) == phi2temp[block2].end()) {
                    
                    vector<pair<AS_operand*,Temp_label*>> phi_args;
                    for(auto pre_block : node->preds) {
                        // if(FG_def(bg.mynodes[pre_block]).find(temp.first) != FG_def(bg.mynodes[pre_block]).end()) 
                        phi_args.push_back({AS_Operand_Temp(temp.first), bg.mynodes[pre_block]->info->label});     
                    }
                    L_stm* stm = L_Phi(AS_Operand_Temp(temp.first), phi_args);// result in different AS with same Temp
                    
                    block2->instrs.insert(next(block2->instrs.begin()), stm); 
                    phi2temp[block2].insert(temp.first);
                    if(FG_def(node).find(temp.first) == FG_def(node).end()) {
                        temp.second.insert(block2);
                    }
                }
            }
        }
    }
}



static void Rename_temp(GRAPH::Graph<LLVMIR::L_block*>& bg, GRAPH::Node<LLVMIR::L_block*>* n, unordered_map<Temp_temp*, stack<Temp_temp*>>& Stack) {
   //   Todo
    unordered_map<Temp_temp*, Temp_temp*> new2origin;
    for(auto stm : n->info->instrs) {
        if(stm->type != L_StmKind::T_PHI) {
            list<AS_operand**> AS_list = get_use_int_operand(stm);
            for(auto AS : AS_list) {
                if(Stack.find((*AS)->u.TEMP) != Stack.end()) *AS = AS_Operand_Temp(Stack[(*AS)->u.TEMP].top());
            }
        }
        list<AS_operand**> AS_list = get_def_int_operand(stm);
        for(auto AS : AS_list) {
            Temp_temp* temp = Temp_newtemp_int();
            Stack[(*AS)->u.TEMP].push(temp);
            new2origin[temp] = (*AS)->u.TEMP;
            *AS = AS_Operand_Temp(Stack[(*AS)->u.TEMP].top());
        }
    }

    for(auto succ : n->succs) {
        for(auto stm : bg.mynodes[succ]->info->instrs) {
            if(stm->type != L_StmKind::T_PHI) continue;
            for(auto& phi : stm->u.PHI->phis) {
                if(phi.second->name != n->info->label->name) continue;
                if(phi.first->kind == OperandKind::TEMP) {
                    phi.first = AS_Operand_Temp(Stack[phi.first->u.TEMP].top());
                }
            }
        }
    }
    for(auto succ: tree_dominators[n->info].succs) {
        Rename_temp(bg, revers_graph[succ], Stack);
    }
    // for(auto def : FG_def(n)) {
    //     Stack[def].pop();
    // }
    for(auto stm : n->info->instrs) {
        list<AS_operand**> AS_list = get_def_int_operand(stm);
        for(auto AS : AS_list) {
            Stack[new2origin[(*AS)->u.TEMP]].pop();
        }
    }
}

void Rename(GRAPH::Graph<LLVMIR::L_block*>& bg) {
   //   Todo
    //init
    unordered_map<Temp_temp*, stack<Temp_temp*>> Stack;
    for(auto node : bg.mynodes) {
        for(auto temp : FG_def(node.second)) {
            if(temp->type == TempType::INT_TEMP) {
                if(Stack[temp].empty()) Stack[temp].push(Temp_newtemp_int());
            }
        }
    }
    Rename_temp(bg, bg.mynodes[0], Stack);
}