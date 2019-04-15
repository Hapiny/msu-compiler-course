#ifdef __cplusplus
#define export exports
extern "C" {
#include <qbe/all.h>
}
#undef export
#else
#include "qbe/all.h"
#endif

#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <vector>


class Loop {
public:
    Loop(Blk*); 
    
    Blk* loop_start = nullptr;
    Blk* loop_header = nullptr;
    std::set<Blk*> blks = {};
    
    // if this loop inside another loop
    int outer_loop_id = -1;
    // if this loop has another loops inside
    std::vector<int> inside_loop_ids;
    
};

Loop::Loop(Blk* s): loop_start(s) {}

bool is_dominator(Blk* blk, Blk* dominator) {
    bool result = false;
    Blk* tmp = blk;
    for(;;) {
		if(tmp == dominator) {
			result = true;
            break;
        }
        tmp = tmp->idom;
        if (!tmp)
            break;
    }
	return result;
}

void check_instructions(Blk *blk, std::vector<Ins*> &instructions) {
	for(uint i = 0; i < blk->nins; i++) {
        uint op_code = blk->ins[i].op;
        bool cond = (isstore(op_code) or isload(op_code) or ispar(op_code) or op_code == Ocall or op_code == Ovacall or (op_code >= Oalloc4 and op_code < Ocopy));
		if (!cond)
			instructions.push_back(&(blk->ins[i]));
    }
}


bool is_marked(uint var_index, std::vector<Ins*> &instructions, std::unordered_map<Ins*, bool> &marked) {
    bool result = false;
	for(Ins *i : instructions) {
		if(i->to.val == var_index) {
			result =  marked[i];
            break;
        }
    }
	return result;
}

void remove_ins(Blk* blk, std::unordered_map<Ins*, bool> &marked){
	std::vector<Ins*> tmp_ins;	

	for(int i = 0; i < blk->nins; i++) {
		if(!marked[&(blk->ins[i])]) {
			tmp_ins.push_back(&(blk->ins[i]));
        }
    }

    if (tmp_ins.size() != blk->nins) {
        Ins* new_ins = nullptr;
        if (tmp_ins.size()) {
            int i = 0;
            new_ins = new Ins[tmp_ins.size()];
            for(Ins* ins : tmp_ins){
                new_ins[i] = *ins;
                i += 1;
            }
        }
        blk->ins = new_ins;
        blk->nins = tmp_ins.size();
    }   
}


bool is_invariant(Ins& ins, Loop& loop, Fn *fn, std::vector<Ins*>& instructions, 
            std::unordered_map<Ins*, bool>& marked, 
            std::unordered_map<uint, Blk*>& all_defs)
{
    bool result = false;
	if(ins.arg[0].type == RCon and ins.arg[1].type == RCon) {
		return true;
    }

	uint left_operand = ins.arg[0].val;
    uint right_operand = ins.arg[1].val;
	Blk *left_def = nullptr;
    Blk *right_def = nullptr;

    if (left_operand) {
        left_def = all_defs[left_operand];
    }
    if (right_operand) {
        right_def = all_defs[right_operand];
    }

	bool left_invariant = (!left_def or is_marked(left_operand, instructions, marked) or 
                            (left_def != loop.loop_start and loop.blks.find(left_def) == loop.blks.end()));
	bool right_invariant = (!right_def or is_marked(right_operand, instructions, marked) or 
                            (right_def != loop.loop_start and loop.blks.find(right_def) == loop.blks.end()));

    result = left_invariant and right_invariant;
	return result;
}

void loop_blks_fill(Loop &loop, Blk* blk, std::set<Blk*> &visited){
	if(blk == loop.loop_start || visited.find(blk) != visited.end())
		return;
    else {
        loop.blks.insert(blk);
        visited.insert(blk);
        for(uint i = 0; i < blk->npred; i++)
            loop_blks_fill(loop, blk->pred[i], visited);
    }
}

void fill_all_loops(Fn *fn, std::vector<Loop> &loops) {
	for(Blk* blk = fn->start; blk; blk = blk->link) {
        std::vector<Blk*> successors = {};
        successors.push_back(blk->s1);
        successors.push_back(blk->s2);
		for(auto s : successors) {
			if(is_dominator(blk, s)) {
                Loop loop = Loop(s);
                bool flag = false;
                std::set<Blk*> visited; 
                loop_blks_fill(loop, blk, visited);

                for(uint i = 0; i < loops.size(); i++){
                    if (
                        loops[i].loop_start == loop.loop_start
                        && !std::includes(loops[i].blks.begin(), loops[i].blks.end(), loop.blks.begin(), loop.blks.end())
                        && !std::includes(loop.blks.begin(), loop.blks.end(), loops[i].blks.begin(), loops[i].blks.end()) 
                    ) {
                        loops[i].blks.insert(loop.blks.begin(), loop.blks.end());
                        flag = true;
                        break;				
                    }
                }
                // inside loops
                if(!flag)
                    loops.push_back(loop);
            }

		}
	}
}

std::vector<Loop> fill_loops(Fn *fn) {
    // fill blks for each loop
    std::vector<Loop> loops;
	fill_all_loops(fn, loops);
    // for (auto loop : loops) {
    //     std::cout << loop.loop_start->name << std::endl;
    //     std::cout << loop.outer_loop_id << std::endl;
    //     std::cout << "===================" << std::endl;
    // }
    // std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	for(uint i = 0; i < loops.size(); i++){
		Loop& loop = loops[i];
		std::set<int> parents;
		std::set<Blk*> blocks(loop.blks);
		blocks.insert(loop.loop_start);

		for(uint j = 0; j < loops.size(); j++){
			if (i != j) {
                std::set<Blk*> another_blks(loops[j].blks);
                another_blks.insert(loops[j].loop_start);

                // for (auto b : blocks)
                //     std::cout << b->name << std::endl;
                // std::cout << std::endl;
                // for (auto b : another_blks)
                //     std::cout << b->name << std::endl;
                // std::cout << "++++++++++++++++++++++++++++" << std::endl;
                if (std::includes(another_blks.begin(), another_blks.end(), blocks.begin(), blocks.end())) {
                    // std::cout << i << " " << j << std::endl;
                    parents.insert(j);
                }
            }
		}
		int size = -1; 
        int outer_loop_id = -1;
		for(int p_idx : parents){
            // std::cout << "____" << std::endl;
            // std::cout << p_idx << std::endl;
            // std::cout << "____" << std::endl;
			if(loops[p_idx].blks.size() < size or size < 0) {
				outer_loop_id = p_idx;
				size = loops[p_idx].blks.size();
			}
		}
		if(outer_loop_id != -1) { 
			loop.outer_loop_id = outer_loop_id;
			loops[outer_loop_id].inside_loop_ids.push_back(i);
		}
	}
	return loops;
}

void create_preheader(int loop_index, std::vector<Loop>& loops, Fn* fn, std::unordered_map<uint, Blk*>& all_defs) {
	Loop& loop = loops[loop_index];
	
    for (int i : loop.inside_loop_ids) {
		create_preheader(i, loops, fn, all_defs);
    }

	std::vector<Ins*> instructions;	
	check_instructions(loop.loop_start, instructions);
	for (Blk* blk : loop.blks) {
		check_instructions(blk, instructions);
	}

	bool changed = true;
	std::unordered_map<Ins*, bool> marked;
	while(changed) {
		changed = false;
		for (Ins* i: instructions) {
			if (!marked[i]) {
                if (is_invariant(*i, loop, fn, instructions, marked, all_defs)) {
                    changed = true;
                    marked[i] = true;
                }
            }
		}
	}
	int num_marked = 0;
	for(auto b : marked)
		if (b.second)
			num_marked++;
	if (num_marked) {
        Blk* header = new Blk;
        int char_num = 0;
        std::string name_str = "prehead@" + std::string(loop.loop_start->name);
        for (char ch: name_str) {
            header->name[char_num] = ch;
            char_num += 1;
        }
        int tmp = 0;
        header->nins = num_marked;
        header->ins = new Ins[num_marked];
        header->jmp.type = Jjmp;
        header->link = loop.loop_start;
        header->phi = nullptr;
        header->s1 = loop.loop_start;
        for(Ins* ins: instructions) {
            if(marked[ins]) {
                header->ins[tmp] = *ins;
                tmp += 1;
            }
        }
        // std::cout << loop.loop_start->name << std::endl;
        remove_ins(loop.loop_start, marked);
        for(Blk* blk : loop.blks) {
            remove_ins(blk, marked);
        }
        if(loop.outer_loop_id >= 0) {
            loops[loop.outer_loop_id].blks.insert(header);
        }
        loop.loop_header = header;
    }
}

void fix_preheder(int loop_index, std::vector<Loop>& loops, std::vector<Blk*>& all_blocks) {
	Loop& loop = loops[loop_index];
	for(int i : loop.inside_loop_ids)
		fix_preheder(i, loops, all_blocks);
	if(loop.loop_header and loop.loop_header->nins) {
    	for (Blk* blk: all_blocks) {
	    	if(blk != loop.loop_start) {
                if(blk->s1 == loop.loop_start and loop.blks.find(blk) == loop.blks.end()) {
                    blk->s1 = loop.loop_header;
                }
                if(blk->s2 == loop.loop_start and loop.blks.find(blk) == loop.blks.end()) {
                    blk->s2 = loop.loop_header;
                }
                if(blk->link == loop.loop_start) {
                    blk->link = loop.loop_header;
                }
            }
	    }
	    all_blocks.push_back(loop.loop_header);
    }
}

static void readfn (Fn *fn) {
    fillrpo(fn);
    fillpreds(fn);
    filluse(fn);
    ssa(fn);

    // collect all variable defs
	std::unordered_map<uint, Blk*> all_defs;
    std::vector<Blk*> all_blocks;
	for(Blk* blk = fn->start; blk; blk = blk->link) {
        // defs from instructions in block 
		for(uint i = 0; i < blk->nins; i++)
			all_defs[blk->ins[i].to.val] = blk;
        // from phi functions in block
		for(Phi* phi = blk->phi; phi; phi = phi->link)
			all_defs[phi->to.val] = blk;
        all_blocks.push_back(blk);
	}
    // fill loop classes
    std::vector<Loop> loops = fill_loops(fn);
    // for (auto loop : loops) {
    //     std::cout << loop.loop_start->name << std::endl;
    //     std::cout << loop.outer_loop_id << std::endl;
    //     std::cout << "===================" << std::endl;
    // }
    for(int i = 0; i < loops.size(); i++) {
        // if this loop is outer create preheader
		if(loops[i].outer_loop_id < 0) {
            create_preheader(i, loops, fn, all_defs);
    		fix_preheder(i, loops, all_blocks);
        }
	}

    // Loop l = Loop(fn->start, fn->start);
    // std::cout << l.outer_loop_id << " "
    //           << l.inside_loop_ids.size() << " " 
    //           << l.blks.size() << " "
    //           << std::endl;
    // for(auto x: all_defs) {
    //     std::cout << x.first << " " <<  x.second->name << std::endl;
    // }

    // for (Blk* b1=fn->start; b1; b1=b1->link) {
    //     // if (!b1->dom)
    //     //     continue;
    //     fprintf(stderr, "%10s:", b1->name);
    //     for (Blk* b=b1->dom; b; b=b->dlink)
    //         fprintf(stderr, " %s", b->name);
    //     fprintf(stderr, "\n");   
    // }
    // std::cout << is_dominator(fn->start->link->link) << std::endl;
    // std::cout << is_dominator(fn->start->link) << std::endl;
    printfn(fn, stdout);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}