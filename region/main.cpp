#ifdef __cplusplus
#define export exports
extern "C" {
#include "../qbe/all.h"
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
    printfn(fn, stdout);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}