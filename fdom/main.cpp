#ifdef __cplusplus
#define export exports
extern "C" {
#include "qbe/all.h"
}
#undef export
#else
#include "qbe/all.h"
#endif
#include <iostream>
#include <unordered_map>

/*
    Keith Cooper, Timothy Harvey, and Ken Kennedy. 
    A simple, fast dominance algorithm. 
    Rice University, CS Technical Report 06-33870, 01 2006
*/

// function intersect(b1, b2) returns node
//      finger1 ← b1
//      finger2 ← b2
//      while (finger1 != finger2)
//          while (finger1 < finger2)
//              finger2 = doms[finger2]
//          while (finger2 < finger1)
//              finger1 = doms[finger1]
//      return finger1

Blk* intersect(Blk* b1, Blk* b2, std::unordered_map<Blk*, Blk*> &doms) {
    if (!b1) return b2;
    if (!b2) return b1;
    auto finger1 = b1, finger2 = b2;
    while (finger1->id != finger2->id) {
        while (finger1->id < finger2->id) {
            // std::cout << "1" << std::endl;
            finger2 = doms[finger2];
        }
        while (finger2->id < finger1->id) {
            // std::cout << "2" << std::endl;
            finger1 = doms[finger1];
        }
    }
    return finger1;
}

// for all nodes, b /* initialize the dominators array */
//      doms[b] ← Undefined
// doms[start node] ← start node
// Changed ← true
// while (Changed)
//      Changed ← false
//      for all nodes, b, in reverse postorder (except start node)
//          new idom ← first (processed) predecessor of b /* (pick one) */
//          for all other predecessors, p, of b
//              if doms[p] != Undefined /* i.e., if doms[p] already calculated */
//                  new idom ← intersect(p, new idom)
//          if doms[b] != new idom
//              doms[b] ← new idom
//              Changed ← true

void simple_fast_dominators(Fn* fn, std::unordered_map<Blk*, Blk*> &doms) {
    doms[fn->start] = fn->start;
    bool changed = true;
    while (changed) {
        changed = false;
        uint all_nodes_num = fn->nblk;
        Blk** rpo_blocks = fn->rpo;
        
        for (uint b = 0; b < all_nodes_num; b++) {
            Blk* blk = rpo_blocks[b];
            // std::cout << (std::string)blk->name << std::endl;
            Blk** predecessors = blk->pred;
            uint predecessors_num = blk->npred;
            Blk* new_idom = nullptr;
            
            
            for (uint p = 0; p < predecessors_num; p++) {
                Blk* pred_blk = predecessors[p];
                if (doms[pred_blk]) {
                    new_idom = intersect(pred_blk, new_idom, doms);
                }
            }

            if (new_idom && doms[blk] != new_idom) {
                doms[blk] = new_idom;
                changed = true;
            }
        }
    }
}

static void readfn (Fn *fn) {
	fillpreds(fn);
	fillrpo(fn);

    // init doms
    std::unordered_map<Blk*, Blk*> doms;
    // fill doms
    simple_fast_dominators(fn, doms);
    // print result
    // std::cout << "END!!!" << std::endl;
    // std::cout << doms.size() << std::endl;
    // for ( auto it = doms.begin(); it != doms.end(); ++it )
    //     std::cout << " " << it->first << " : " << it->second;
    // std::cout << std::endl;

	for(Blk* blk = fn->start; blk; blk = blk->link) {
        std::string blk_name = "@" + (std::string)blk->name;
        std::string dom_name = "@" + (std::string)doms[blk]->name;
        std::cout << blk_name << "\t" << dom_name << std::endl;
    }
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, (char *)"<stdin>", readdat, readfn);
  freeall();
}