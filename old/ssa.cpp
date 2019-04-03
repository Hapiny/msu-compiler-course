#ifdef __cplusplus
#define export exports
extern "C" {
#include <qbe/all.h>
}
#undef export
#else
#include <qbe/all.h>
#endif

#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <queue>

using namespace std;

struct Phi_func {
    string var;
    int index;
    vector<string> args;
};

template<typename C,typename T>
bool contains(C v, T elem) {
    if (find(v.begin(), v.end(), elem) == v.end())  
        return false;
    else
        return true;
}

bool is_equal_vectors(vector<string> &v1, vector<string> &v2) {
    set<string> s1(v1.begin(), v1.end());
    set<string> s2(v2.begin(), v2.end());
    if (s1 != s2) 
        return false;
    else
        return true;
}

vector<string> vec_intersect(vector<string> &v1, vector<string> &v2) {
    vector<string> result;
    for (auto elem : v1) {
        if (find(v2.begin(), v2.end(),elem) != v2.end())
            result.push_back(elem);
    }
    return result;
}

set<string> my_union(set<string> &s1, set<string> &s2) {
    set<string> result;
    set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(result, result.end()));
    return result;
}

Blk* get_blk_by_name(Fn* fn, string blk_name) {
    Blk* result = NULL;
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        string tmp = blk->name;
        if (tmp == blk_name) {
            result = blk;
            break;
        }
    }
    return result;
}

void build_blocks_global(Fn* fn, set<string> &globals, map<string, vector<string>> &blocks) {
    bool first_block = true;
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        set<string> use;
        set<string> def;
        string blk_name = blk->name;
        
        for (uint i = 0; i < blk->nins; i++) {
            Ins *inst = &blk->ins[i];
            string left = fn->tmp[inst->arg[0].val].name;
            string right = fn->tmp[inst->arg[1].val].name;
            string assign_var = fn->tmp[inst->to.val].name;

            if (left.length() && !contains(def, left))
                use.insert(left);
            if (right.length() && !contains(def, right)) 
                use.insert(right);
            if (assign_var.length()) {
                def.insert(assign_var);
                blocks[assign_var].push_back(blk_name); 
            }
        }

        if (first_block) 
            use.clear();
        first_block = false;

        if (!blk->s1 && !blk->s2) {
            string ret_arg = fn->tmp[blk->jmp.arg.val].name;
            if (ret_arg.length() && !contains(def, ret_arg)) 
                use.insert(ret_arg);
        }
        globals = my_union(globals, def);
        globals = my_union(globals, use);
    }
}

vector<string> build_pred_succ(Fn *fn, map<string, vector<string>> &p, map<string, vector<string>> &s) {
    vector<string> all_blocks;
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        string blk_name = blk->name;
        all_blocks.push_back(blk_name);
        string s1, s2;
        if (blk->s1) {
            s1 = blk->s1->name;
            s[blk_name].push_back(s1);
            p[s1].push_back(blk_name);
        }  
        if (blk->s2) {
            s2 = blk->s2->name;
            s[blk_name].push_back(s2);
            p[s2].push_back(blk_name);
        }
    }
    return all_blocks;
}

string init_dom(Fn *fn, map<string,vector<string>> &dom, vector<string> &all_blocks) {
    bool first_block = true;
    string first_blk_name;
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        string blk_name = blk->name;
        if (first_block) {
            first_block = false;
            first_blk_name = blk_name;
            dom[first_blk_name].push_back(first_blk_name);
            continue;
        }
        dom[blk_name] = all_blocks;
    }
    return first_blk_name;
}

void build_dominators(Fn *fn, map<string,vector<string>> &dom, map<string,vector<string>> &dom_rev) {
    map<string,vector<string>> predecessor, s;
    vector<string> all_blocks = build_pred_succ(fn, predecessor, s);
    string first_blk_name = init_dom(fn, dom, all_blocks);
    bool change = true;
    while (change) {
        change = false;
        bool any_change = false;
        for (int i = 1; i < all_blocks.size(); ++i) {
            vector<string> new_dom = all_blocks;
            string blk_name = all_blocks[i];
            // Intersection
            for (auto pred_block : predecessor[blk_name]) 
                new_dom = vec_intersect(new_dom, dom[pred_block]);
            
            // Union
            if (find(new_dom.begin(), new_dom.end(),blk_name) == new_dom.end())
                new_dom.push_back(blk_name);

            if (!is_equal_vectors(dom[blk_name], new_dom)) {
                any_change = true;
                dom[blk_name] = new_dom;
            }
        }
        if (any_change)
            change = true;
    }
    for (int i = 0; i < all_blocks.size(); ++i) {
        string blk_name = all_blocks[i];
        for (auto block : dom[all_blocks[i]])  
            dom_rev[block].push_back(blk_name);  
    }
}

bool is_idom(map<string,vector<string>> &dom, string node, string candidate) {
    vector<string> node_dominators = dom[node];
    node_dominators.erase(find(node_dominators.begin(), node_dominators.end(), node));
    node_dominators.erase(find(node_dominators.begin(), node_dominators.end(), candidate));
    for (auto x : node_dominators) {
        if (!contains(dom[candidate], x))
            return false;
    }
    return true;
} 

map<string, string> build_idoms(map<string, vector<string>> &doms, vector<string> &all_blocks) {
    map<string,string> result;
    for (int i = 0; i < all_blocks.size(); ++i) {
        if (i == 0) {
            result[all_blocks[i]] = "EMPTY";
        }
        else {
            for (auto block : doms[all_blocks[i]]) {
                if (is_idom(doms, all_blocks[i], block)) {
                    result[all_blocks[i]] = block;
                    break;
                }
            }
        }
    }
    return result;
}

void init_df(map<string, set<string>> &df, vector<string> &all_blocks) {
    for(int i = 0; i < all_blocks.size(); ++i) {
        df[all_blocks[i]] = {};
    }
}

map<string,string> build_dominance_frontier(Fn *fn, map<string, set<string>> &dominance_frontier) {
    map<string,vector<string>> predecessor, s; 
    vector<string> all_blocks =  build_pred_succ(fn, predecessor, s);

    map<string,vector<string>> dominators, reverse_dominators;
    build_dominators(fn, dominators, reverse_dominators);
    map<string,string> idoms = build_idoms(dominators, all_blocks);

    init_df(dominance_frontier, all_blocks);

    for (int i = 0; i < all_blocks.size(); ++i) {
        if (predecessor[all_blocks[i]].size() > 1) {
            for (auto pred : predecessor[all_blocks[i]]) {
                string tmp = pred;
                while (tmp != idoms[all_blocks[i]]) {
                    set<string> n = {all_blocks[i]};
                    dominance_frontier[tmp] = my_union(dominance_frontier[tmp], n);
                    tmp = idoms[tmp];
                }
            }
        }
    }
    return idoms;
}

map<string, int> counters;
map<string, stack<int>> stacks;
map<string, vector<string>> succ, dom_tree;
map<string, vector<Phi_func>> phi_functions;
map<string, vector<string>> blocks, pred;
set<string> globals;

int new_name(string n) {
    int i = counters[n];
    counters[n] += 1;
    stacks[n].push(i);;
    return i;
}


void rename(Fn* fn, string blk_name) {
    vector<string> s = succ[blk_name];
    for (int i = 0; i < phi_functions[blk_name].size(); i++) {
        int new_ = new_name(phi_functions[blk_name][i].var);
        phi_functions[blk_name][i].index = new_;
    }
    
    Blk *blk = get_blk_by_name(fn, blk_name);
    for (uint i = 0; i < blk->nins; i++) {
        Ins* instr = &blk->ins[i];
        string assign_var = fn->tmp[instr->to.val].name;
        if (assign_var.length())
            new_name(assign_var);
    }

    for (auto b : s) {
        for(int i = 0; i < phi_functions[b].size(); i++) {
            string var_name = phi_functions[b][i].var;
            int ssa_index;
            if (stacks[var_name].empty())
                ssa_index = -1;
            else 
                ssa_index = stacks[var_name].top();
            phi_functions[b][i].args.push_back(blk_name + " " + to_string(ssa_index)); 
        }
    }

    for (auto b : dom_tree[blk_name])
        rename(fn, b);
    
    for (int i = 0; i < phi_functions[blk_name].size(); i++) {
        Phi_func p = phi_functions[blk_name][i];
        stacks[p.var].pop();
    }
    
    for (uint i = 0; i < blk->nins; i++) {
        Ins* instr = &blk->ins[i];
        string assign_var = fn->tmp[instr->to.val].name;
        stacks[assign_var].pop();
    }
}


static void readfn (Fn *fn) {
    map<string, set<string>> dom_frontier;
    build_blocks_global(fn, globals, blocks);
    vector<string> all_blocks = build_pred_succ(fn, pred, succ);
    map<string,string> idoms = build_dominance_frontier(fn, dom_frontier);
    string root;
    // init phi functions
    for (auto blk : all_blocks) {
        phi_functions[blk] = {}; 
        dom_tree[blk] = {};
    }
    // fill dominators tree
    for (auto p : idoms) {
        if (p.second != "EMPTY")
            dom_tree[p.second].push_back(p.first);
        else
            root = p.first;
    }

    // insert phi functions
    map<string, set<string>> marked;
    for (auto x : globals) {
        queue<string> worklist;
        for (auto b : blocks[x])
            worklist.push(b);
        while (!worklist.empty()) {
            string blk = worklist.front();
            worklist.pop();
            for (auto b_df : dom_frontier[blk]) {
                vector<string> phi_vars_in_df;
                for (auto phi : phi_functions[b_df])
                    phi_vars_in_df.push_back(phi.var);
                if (!contains(marked[x], b_df)) {
                    if (!contains(phi_vars_in_df, x)) {
                        Phi_func p = { .var = x, .index = 0, .args = {}};
                        phi_functions[b_df].push_back(p);
                    }
                    worklist.push(b_df);
                    marked[x].insert(b_df);
                }
            }
        } 
    }

    //init counters and stacks
    for (auto x : globals) {
        counters[x] = 1;
        stacks[x] = {};
    }
    rename(fn, root);

    for (Blk* blk = fn->start; blk; blk = blk->link) {
        printf("@%s:\n", blk->name);
        string blk_name = blk->name;
        for (auto p : phi_functions[blk_name]) {
            cout << "%" << p.var <<  " " << p.index << " = ";
            for (auto arg : p.args)
                cout << "@" << arg << " ";
            cout << endl; 
        }
    }
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}