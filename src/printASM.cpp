#include "printASM.h"
#include "asm_arm.h"
#include <iostream>

using namespace std;
using namespace ASM;

void ASM::printAS_global(std::ostream &os, ASM::AS_global *global) {
    // Fixme: add here
    os << global->label->name << ":\n";
    if(global->len == 0)
    {
        os << "\t.word " << global->init << "\n";
    }
    else {
        os << "\t.zero " << global->len << "\n";
    }
}

void ASM::printAS_decl(std::ostream &os, ASM::AS_decl *decl) {
    // Fixme: add here
    os << ".global " << decl->name << "\n";
}


void ASM::printAS_stm(std::ostream &os, AS_stm *stm) {
    // Fixme: add here
    switch (stm->type)
    {
        case AS_stmkind::BINOP:
            os << "\t";
            switch(stm->u.BINOP->op)
            {
                case AS_binopkind::ADD_:
                    os << "add";
                    break;
                case AS_binopkind::SUB_:
                    os << "sub";
                    break;
                case AS_binopkind::MUL_:
                    os << "mul";
                    break;
                case AS_binopkind::SDIV_:
                    os << "sdiv";
                    break;
                default:
                    break;
            }
            os << "\t";
            printAS_reg(os, stm->u.BINOP->dst);
            os << ", ";
            printAS_reg(os, stm->u.BINOP->left);
            os << ", ";
            printAS_reg(os, stm->u.BINOP->right);
            os << "\n";
            break;
        case AS_stmkind::MOV:
            os << "\tmov\t";
            printAS_reg(os, stm->u.MOV->dst);
            os << ", ";
            printAS_reg(os, stm->u.MOV->src);
            os << "\n";
            break;
        case AS_stmkind::LDR:
            os << "\tldr\t";
            printAS_reg(os, stm->u.LDR->dst);
            os << ", [";
            printAS_reg(os, stm->u.LDR->ptr);
            os << "]\n";
            break;
        case AS_stmkind::STR:
            os << "\tstr\t";
            printAS_reg(os, stm->u.STR->src);
            os << ", [";
            printAS_reg(os, stm->u.STR->ptr);
            os << "]\n";
            break;
        case AS_stmkind::LABEL: 
            os << stm->u.LABEL->name << ":\n";
            break;
        case AS_stmkind::B: 
            os << "\tb\t" << stm->u.B->jump->name << "\n";
            break;
        case AS_stmkind::BCOND: 
            os << "\tb.";
            switch(stm->u.BCOND->op)
            {
                case AS_relopkind::EQ_:
                    os << "eq";
                    break;
                case AS_relopkind::NE_:
                    os << "ne";
                    break;
                case AS_relopkind::LT_:
                    os << "lt";
                    break;
                case AS_relopkind::GT_:
                    os << "gt";
                    break;
                case AS_relopkind::LE_:
                    os << "le";
                    break;
                case AS_relopkind::GE_:
                    os << "ge";
                    break;
                default:
                    break;
            }
            os << "\t";
            os << stm->u.BCOND->jump->name << "\n";
            break;
        case AS_stmkind::CMP:
            os << "\tcmp\t";
            printAS_reg(os, stm->u.CMP->left);
            os << ", ";
            printAS_reg(os, stm->u.CMP->right);
            os << "\n";
            break;
        case AS_stmkind::RET:
            os << "\tret\n";
            break;
        case AS_stmkind::ADR:
            os << "\tadr\t";
            printAS_reg(os, stm->u.ADR->reg);
            os << ", " << stm->u.ADR->label->name << "\n";
            break;
        default:
            break;
    }
}

void ASM::printAS_reg(std::ostream &os, AS_reg *reg) {
    // Fixme: add here
    if(reg->reg >= 0) {
        os << "x" << reg->reg;
    }
    else if(reg->reg == -3) {
        os << "#" << reg->offset;
    }
    else if(reg->reg == -1) {
        if(reg->offset == -1 || reg->offset == 0) {
            os << "sp";
        }
        else {
            os << "sp, #" << reg->offset;
        }
    }

}

void ASM::printAS_func(std::ostream &os, AS_func *func) {
    for(const auto &stm : func->stms) {
        printAS_stm(os, stm);
    }
}

void ASM::printAS_prog(std::ostream &os, AS_prog *prog) {

    os << ".section .data\n";
    for(const auto &global : prog->globals) {
        printAS_global(os, global);
    }

    os << ".section .text\n";

    for(const auto &decl : prog->decls) {
        printAS_decl(os, decl);
    }

    for(const auto &func : prog->funcs) {
        printAS_func(os, func);
    }

}