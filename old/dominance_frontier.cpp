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
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <iostream>

using namespace std;


vector<string> vec_intersect(vector<string> &v1, vector<string> &v2) {
    vector<string> result;
    for (auto elem : v1) {
        if (find(v2.begin(), v2.end(),elem) != v2.end())
            result.push_back(elem);
    }
    return result;
}

bool contains(vector<string> &v, string elem) {
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

vector<string> build_pred(Fn *fn, map<string, vector<string>> &p) {
    vector<string> all_blocks;
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        string blk_name = blk->name;
        all_blocks.push_back(blk_name);
        string s1, s2;
        if (blk->s1) {
            s1 = blk->s1->name;
            p[s1].push_back(blk_name);
        }  
        if (blk->s2) {
            s2 = blk->s2->name;
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
    map<string,vector<string>> predecessor;
    vector<string> all_blocks = build_pred(fn, predecessor);
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

set<string> my_union(set<string> &s1, set<string> &s2) {
    set<string> result;
    set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(result, result.end()));
    return result;
}
    
static void readfn (Fn *fn) {
    map<string,vector<string>> predecessor; 
    vector<string> all_blocks =  build_pred(fn, predecessor);

    map<string,vector<string>> dominators, reverse_dominators;
    build_dominators(fn, dominators, reverse_dominators);
    map<string,string> idoms = build_idoms(dominators, all_blocks);

    map<string, set<string>> dominance_frontier;
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

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        printf("@%s:\t", blk->name);
        for (auto block : dominance_frontier[blk->name]) {
            printf("@%s ", block.c_str());
        }
        printf("\n");
    }
}

static void readdat (Dat *dat) {
    (void) dat;
}

int main () {
    parse(stdin, "<stdin>", readdat, readfn);
    freeall();
}
