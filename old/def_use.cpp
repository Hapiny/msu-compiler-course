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
#include <set>
#include <string>


static void readfn (Fn *fn) {
  bool first_block = true;
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    std::set<std::string> use;
    std::set<std::string> def;
    printf("@%s", blk->name);

    for (uint i = 0; i < blk->nins; i++) {
      Ins *inst = &blk->ins[i];
      std::string left = fn->tmp[inst->arg[0].val].name;
      std::string right = fn->tmp[inst->arg[1].val].name;
      std::string assign_var = fn->tmp[inst->to.val].name;

      if (left.length()) {  
        if (def.find(left) == def.end())
          use.insert(left);
      }  
      if (right.length()) {
        if (def.find(right) == def.end())
          use.insert(right);
      }
      if (assign_var.length())
        def.insert(assign_var);
    }

    if (first_block)
      use.clear();
    first_block = false;

    if (!blk->s1 && !blk->s2) {
        std::string ret_instr_arg = fn->tmp[blk->jmp.arg.val].name;
        if (ret_instr_arg.length()) {
          if (def.find(ret_instr_arg) == def.end())
          use.insert(ret_instr_arg);
        }
    }

    printf("\n\tdef = ");
    for (auto elem : def)
      printf("%%%s ", elem.c_str());

    printf("\n\tuse = ");
    for (auto elem : use)
      printf("%%%s ", elem.c_str());
    printf("\n\n");
  }
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}