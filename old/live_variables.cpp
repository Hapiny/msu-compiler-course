#ifdef __cplusplus
#define export exports
extern "C" {
#include "qbe/all.h"
}
#undef export
#else
#include "qbe/all.h"
#endif

#include <stdio.h>
#include <set>
#include <map>
#include <algorithm>
#include <queue>
#include <deque>
#include <string>
#include <iostream>

std::deque<std::string> build_succ_pred(Fn *fn, std::map<std::string,std::set<std::string>> &s, 
                                   std::map<std::string,std::set<std::string>> &d) {
  std::deque<std::string> worklist;
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    std::string blk_name = blk->name;
    worklist.push_back("@"+blk_name);
    std::string s1, s2;
    if (blk->s1) {
      s1 = blk->s1->name;
      s["@" + blk_name].insert("@"+s1);
      d["@" + s1].insert("@" + blk_name);
    }  
    if (blk->s2) {
      s2 = blk->s2->name;
      s["@" + blk_name].insert("@"+s2);
      d["@" + s2].insert("@" + blk_name);
    }
  }
  return worklist;
}

void build_def_use(Fn *fn, std::map<std::string,std::pair<std::set<std::string>,std::set<std::string>>> &du) {
  bool first_block = true;
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    std::set<std::string> use;
    std::set<std::string> def;
    std::string blk_name = blk->name;

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
    du["@" + blk_name].first = def;
    du["@" + blk_name].second = use; 
  }
  du["Exit"].first = {};
  du["Exit"].second = {};
}

void init_live_variabls(std::deque<std::string> worklist, std::map<std::string,std::set<std::string>> &lv) {
  while(not worklist.empty()) {
    lv[worklist.front()] = {}; 
    worklist.pop_front();
  }
}

static void readfn (Fn *fn) {
  std::map<std::string,std::set<std::string>> successor, predecessor, live_variables;
  std::map<std::string, std::pair<std::set<std::string>,std::set<std::string>>> def_use;

  auto worklist = build_succ_pred(fn, successor, predecessor);
  build_def_use(fn, def_use);

  init_live_variabls(worklist, live_variables);

  while (!worklist.empty()) {
    std::string current_block_name = worklist.back();
    worklist.pop_back();

    std::set<std::string> new_out_block;
    for (auto block : successor[current_block_name]) {
      std::set<std::string> tmp_new;
      if (live_variables[block].empty()) {
        std::set_union(new_out_block.begin(), new_out_block.end(), 
                       def_use[block].second.begin(), def_use[block].second.end(), 
                       std::inserter(tmp_new, tmp_new.end()));
        new_out_block = tmp_new;
      }
      else {
        std::set<std::string> diff, tmp_union;
        std::set_difference(live_variables[block].begin(), live_variables[block].end(),
                            def_use[block].first.begin(), def_use[block].first.end(), 
                            std::inserter(diff, diff.end()));
        std::set_union(def_use[block].second.begin(), def_use[block].second.end(),
                       diff.begin(), diff.end(), 
                       std::inserter(tmp_union, tmp_union.end()));
        std::set_union(tmp_union.begin(), tmp_union.end(),
                        new_out_block.begin(), new_out_block.end(),
                        std::inserter(tmp_new, tmp_new.end()));
        new_out_block = tmp_new;
      }
    }

    std::set<std::string>tmp;
    std::set_difference(new_out_block.begin(), new_out_block.end(),
                        live_variables[current_block_name].begin(), live_variables[current_block_name].end(),
                        std::inserter(tmp, tmp.end()));
    if (!tmp.empty()) {
      live_variables[current_block_name] = new_out_block;
      worklist.push_front(current_block_name);
      for (auto block : predecessor[current_block_name])
        worklist.push_front(block);
    }
  }

  for (Blk *blk = fn->start; blk; blk = blk->link) {
    printf("@%s", blk->name);
    printf("\n\tlv_out = ");
    std::string name = blk->name;
    for (auto var : live_variables["@" + name]) {
      std::cout << "%" <<var << " ";
    }
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
