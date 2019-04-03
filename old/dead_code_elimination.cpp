#ifdef __cplusplus
#define export exports
extern "C" {
#include "qbe/all.h"
}
#undef export
#else
#include "qbe/all.h"
#endif


#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
using namespace std;

typedef Blk* Node;
typedef Ins* Instruction;
typedef Phi* Phi_Function;

enum operation_type { JUMP,
                      PHI,
                      INSTRUCTION,
                      other,
};

struct Operation_Item {
    enum operation_type type;
    Node blk;
    Node jmp_blk;
    Instruction instr;
    Phi_Function phi_f;
};

struct Block_Info {
    vector<Node> pred = {};
    vector<Node> dom = {};
    Node idom = nullptr;
    vector<Node> dom_frontier = {};
    bool is_critical = false;
    vector<Phi_Function> critical_phi = {};
    vector<Instruction> critical_instuctions = {};
};

typedef vector<pair<Node,Block_Info*>> Fn_Info;

Block_Info* get_block_info(Node n, Fn_Info &fn_info) {
    for (auto pair : fn_info) {
        if (pair.first == n)
            return pair.second;
    }
}

template<typename T>
bool is_in_block(T n, vector<T> v) {
    bool result = false;
    for (auto elem : v) {
        if (elem == n) {
            result = true;
            break;
        }
    }
    return result;
}

bool any_change(Fn *fn, Node blk, Fn_Info & fn_info) {
    bool change = false;
    vector<Node> new_val = {};
    Block_Info* blk_info = get_block_info(blk, fn_info);

    new_val.push_back(blk);

    vector<Node> tmp;

    for (Node pred_blk : blk_info->pred) {
        Block_Info* pred_blk_info = get_block_info(pred_blk, fn_info);
        for (Node pred_dom : pred_blk_info->dom) {
            bool flag = true;
            for (Node another_pred : blk_info->pred) {
                if (another_pred == blk or another_pred == pred_blk) {
                    continue;
                }
                else {
                    Block_Info* another_pred_info = get_block_info(another_pred, fn_info);
                    if (is_in_block(pred_dom, another_pred_info->dom) == false) {
                        flag = false;
                        break;
                    }
                }
            }
            if (flag)
                tmp.push_back(pred_dom);
        }
    }

    for (Node elem : tmp)
        new_val.push_back(elem);
    bool flag = false;
    for (Node old : blk_info->dom) {
        if (is_in_block(old, new_val) == false) {
            flag = true;
            break;
        }
    }
    for (Node new_blk : new_val) {
        if (is_in_block(new_blk, blk_info->dom) == false) {
            flag = true;
            break;
        }
    }
    if (flag) {
        change = true;
        blk_info->dom = new_val;
    }
    return change;
}

void build_pred(Fn* fn, Fn_Info& fn_info) {
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        for (int i = 0; i < blk->npred; ++i) {
            Node parent = blk->pred[i];
            Block_Info* blk_info = get_block_info(parent, fn_info);
            blk_info->pred.push_back(blk); 
        }
    }
}

void build_dominators(Fn* fn, Fn_Info& fn_info) {
    Node first_block = fn->start;
    bool change = true;
    Block_Info* first_blk_info = get_block_info(first_block, fn_info);
    first_blk_info->dom.push_back(first_block);

    if (first_block->link) {
        for (Node blk = first_block->link; blk; blk=blk->link) {
            Block_Info* blk_info = get_block_info(blk, fn_info);
            blk_info->dom.push_back(blk); //own dominator
            for (Node tmp = first_block; tmp; tmp = tmp->link) {
                if (is_in_block(tmp, blk_info->dom) == false)
                    blk_info->dom.push_back(tmp);
            }
        }
    }

    while (change) {
        change = false;
        for (Node blk = first_block; blk; blk = blk->link) {
            if (any_change(fn, blk, fn_info)) {
                change = true;
            }
        }
    }
}

void build_idoms(Fn* fn, Fn_Info& fn_info) {
    for (Node blk = fn->start; blk; blk = blk->link) {
        Block_Info* blk_info = get_block_info(blk, fn_info);
        if (blk_info->dom.size() == 1)
            continue;
        for (int i = 0; i < blk_info->dom.size(); ++i) {
            Node candidate = blk_info->dom[i];
            if (candidate != blk) {
                bool flag = true;
                for (int j = 0; j < blk_info->dom.size(); ++j) {
                    Node tmp = blk_info->dom[j];
                    if (tmp != blk and tmp != candidate) {
                        Block_Info *tmp_info = get_block_info(tmp, fn_info);
                        if (is_in_block(candidate, tmp_info->dom) == true) {
                            flag = false;
                            break;
                        }
                    }
                }
                if (flag) {
                    blk_info->idom = candidate;
                    break;
                }
            }
        }
    }
}

void build_dominance_frontier(Fn* fn, Fn_Info & fn_info) {
    for (Node blk = fn->start; blk; blk= blk->link) {
        Block_Info* blk_info = get_block_info(blk, fn_info);
        if (blk_info->pred.size()) {
            for (int i = 0; i < blk_info->pred.size(); i++) {
                Node r = blk_info->pred[i];
                Block_Info* r_info = get_block_info(r, fn_info);
                while (r != blk_info->idom) {
                    if (!is_in_block(blk, r_info->dom_frontier)) {
                        r_info->dom_frontier.push_back(blk);
                    }
                    r = r_info->idom;
                    r_info = get_block_info(r, fn_info);
                }
            }
        }
    }
}

Operation_Item def_operand(Fn* fn, string operand) {
    Operation_Item item;
    for (Node blk = fn->start; blk; blk=blk->link) {
        for (Instruction i = blk->ins; i < &blk->ins[blk->nins]; ++i) {
            if (fn->tmp[i->to.val].name == operand) {
                item.blk = blk;
                item.type = INSTRUCTION;
                item.instr = i;
                return item;
            }
        }
        for (Phi_Function phi = blk->phi; phi; phi = phi->link) {
            if (fn->tmp[phi->to.val].name == operand) {
                item.blk = blk;
                item.type = PHI;
                item.phi_f = phi;
                return item;
            }
        }
    }
    item.blk = nullptr;
    item.type = other;
    return item;
}

void prepro_all_blocks(Fn* fn, Fn_Info& fn_info) {
    // init empty fn_info for each block
    Node first_block = fn->start;
    for (Node blk = first_block; blk; blk = blk->link) {
        Block_Info* new_info = new(Block_Info);   
        fn_info.push_back(make_pair(blk, new_info));
    }   


    build_pred(fn, fn_info);
    build_dominators(fn, fn_info);
    build_idoms(fn, fn_info);
    build_dominance_frontier(fn, fn_info);
}

void mark_step(Fn* fn, Fn_Info& fn_info) {
    vector<Operation_Item> worklist;
    for (Node blk = fn->start; blk; blk = blk->link) {
        Block_Info* blk_info = get_block_info(blk, fn_info);
        for (Instruction i = blk->ins; i < &blk->ins[blk->nins]; i++) {
            if (isstore(i->op) || i->op == Ocall || i->op == Ovacall) {
                Operation_Item item;
                item.blk = blk;
                item.type = INSTRUCTION;
                item.instr = i;
                worklist.push_back(item);
            }
        }
        blk_info->is_critical = false;
        if (isret(blk->jmp.type)) {
            Operation_Item item;
            item.blk = blk;
            item.type = JUMP;
            item.jmp_blk = blk;
            worklist.push_back(item);
        }
    }
    while (!worklist.empty()) {
        Operation_Item item = worklist.back();
        worklist.pop_back();
        if (item.type == other)
            continue;
        if (item.type == INSTRUCTION) {
            Block_Info* blk_info = get_block_info(item.blk, fn_info);
            if (is_in_block(item.instr, blk_info->critical_instuctions)) {
                continue;
            }
            blk_info->critical_instuctions.push_back(item.instr);
            if (item.instr->op == Ocall || item.instr->op == Ovacall) {
                Instruction i;
                for (i = item.instr; i > item.blk->ins; i--) {
                    if (!isarg((i-1)->op))
                        break;
                }
                while (i < item.instr) {
                    Operation_Item new_item;
                    new_item.blk = item.blk;
                    new_item.type = INSTRUCTION;
                    new_item.instr = i;
                    worklist.push_back(new_item);
                    i++;
                }
            } else {
                int argc;
                if (item.instr->arg[1].type > 0 || item.instr->arg[1].val >= Tmp0) 
                    argc = 2;
                else
                    argc = 1;
                for (int j = 0; j < argc; j++) {
                    if (item.instr->arg[j].val < Tmp0) {
                        continue;
                    }
                    string operand_name = fn->tmp[item.instr->arg[j].val].name;
                    worklist.push_back(def_operand(fn, operand_name));
                }
            }
        }
        else if (item.type == JUMP) {
            Block_Info* blk_jmp_info = get_block_info(item.jmp_blk, fn_info);
            if (blk_jmp_info->is_critical) {
                continue;
            }
            blk_jmp_info->is_critical = true;
            if (item.jmp_blk->jmp.type > 0 and 
                    (item.jmp_blk->jmp.arg.type > 0 or item.jmp_blk->jmp.arg.val >= Tmp0)) {
                string operand_name = fn->tmp[item.jmp_blk->jmp.arg.val].name;
                worklist.push_back(def_operand(fn, operand_name));
            }
        }
        else if (item.type == PHI){
            Block_Info* blk_info = get_block_info(item.blk, fn_info);
            if (is_in_block(item.phi_f, blk_info->critical_phi))
                continue;
            blk_info->critical_phi.push_back(item.phi_f);
            for (int i = 0; i < item.phi_f->narg; i++) {
                if (item.phi_f->arg[i].val < Tmp0)
                    continue;
                string operand_name = fn->tmp[item.phi_f->arg[i].val].name;
                worklist.push_back(def_operand(fn, operand_name));
                Node phi_blk = item.phi_f->blk[i];
                if (get_block_info(phi_blk, fn_info)->is_critical)
                    continue;
                Operation_Item new_item;
                new_item.blk = phi_blk;
                new_item.type = JUMP;
                new_item.jmp_blk = phi_blk;
                worklist.push_back(new_item);   
            }
        }

        Node blk = item.blk;
        Block_Info* blk_info = get_block_info(blk, fn_info);
        for (Node blk_df : blk_info->dom_frontier) {
            Block_Info* blk_df_info = get_block_info(blk_df, fn_info);
            if (blk_df_info->is_critical) 
                continue;
            Operation_Item new_item;
            new_item.blk = blk_df;
            new_item.type = JUMP;
            new_item.jmp_blk = blk_df;
            worklist.push_back(new_item);
        }
    }

}


void sweep_step(Fn *fn, Fn_Info& fn_info) {
    for (Node blk = fn->start; blk; blk = blk->link) {
        Block_Info* blk_info = get_block_info(blk, fn_info);
        for (Instruction i = blk->ins; i < &blk->ins[blk->nins]; i++) {
            if (is_in_block(i, blk_info->critical_instuctions) == false) {
                i->op = Onop;
                i->to = R;
                i->arg[0] = R;
                i->arg[1] = R;
            }
        }
        Phi_Function previous_phi = nullptr;
        
        for (Phi_Function phi = blk->phi; phi; ) {
            if (is_in_block(phi, blk_info->critical_phi) == false) {
                if (!previous_phi) 
                    blk->phi = phi->link;
                else 
                    previous_phi->link = phi->link;
                phi = phi->link;
            }
            else {
                previous_phi = phi;
                phi = phi->link;
            }
        }

        if (!blk_info->is_critical and blk->jmp.type != Jjmp) {
            blk->jmp.type = Jjmp;
            blk->jmp.arg = R;
            Node new_block = blk_info->idom;
            Block_Info* new_info = get_block_info(new_block, fn_info);
            while(!new_info->is_critical) {
                new_block = new_info->idom;
                new_info = get_block_info(new_block, fn_info);
            }
            blk->s1 = new_block;
            blk->s2 = nullptr;
        }
    }
}

static void readfn (Fn *fn) {
    fillrpo(fn); // Traverses the CFG in reverse post-order, filling blk->id.
    fillpreds(fn);
    filluse(fn);
    ssa(fn);

    Fn_Info fn_info;

    prepro_all_blocks(fn, fn_info);
    mark_step(fn, fn_info);
    sweep_step(fn, fn_info);

    fillpreds(fn); // Either call this, or keep track of preds manually when rewriting branches.
    fillrpo(fn); // As a side effect, fillrpo removes any unreachable blocks.
    printfn(fn, stdout);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}