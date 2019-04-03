#ifdef __cplusplus
#define export exports
extern "C" {
#include "../qbe/all.h"
}
#undef export
#else
#include "../qbe/all.h"
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


static void readfn (Fn *fn) {
    map<string,vector<string>> predecessor, dominators;
    vector<string> all_blocks = build_pred(fn, predecessor);
    string first_blk_name = init_dom(fn, dominators, all_blocks);
    bool change = true;


    // cout << "All Blocks: "; 
    // for (auto block : all_blocks) {
    //     cout << block << " ";
    // }
    // cout << "\n==============PREDECESSORS===================" << endl;
    
    // for (int i = 1; i < all_blocks.size(); ++i) {
    //     cout << "Block name: " << all_blocks[i] << endl;
    //     cout << "    Pred: ";
    //     for (auto block : predecessor[all_blocks[i]]) {
    //         cout << block << " "; 
    //     }
    //     cout << endl;
    // }
    // cout << "==============DOMINATORS=======================" << endl;

    // for (int i = 0; i < all_blocks.size(); ++i) {
    //     cout << "Block name: " << all_blocks[i] << endl;
    //     cout << "    Dominators: ";
    //     for (auto block : dominators[all_blocks[i]]) { 
    //         cout << block << " "; 
    //     }
    //     cout << endl;
    // }
    // cout << "===================Begin While===================\n";
    int iter = 0;
    while (change) {
        iter++;
        change = false;
        bool any_change = false;
        // cout << "\n==========================Iteration #" << iter << "=========================="<< endl;
        for (uint i = 1; i < all_blocks.size(); ++i) {
            vector<string> new_dom = all_blocks;
            string blk_name = all_blocks[i];

            // cout << "\nBlock name: " << blk_name << endl;

            // Intersection
            // cout << "Predecessor Intersection...\n"; 
            for (auto pred_block : predecessor[blk_name]) {

                // cout << "    Pred block: " << pred_block << endl;
                // cout << "    Dominators of pred: \n";
                // cout << "        ";
                // for (auto x : dominators[pred_block])
                //     cout << x << " ";
                // cout << endl;

                new_dom = vec_intersect(new_dom, dominators[pred_block]);

                // cout << "    Intersection: ";
                // for (auto x : new_dom)
                //     cout << x << " ";
                // cout << "\n\n";
            }
            // Union
            if (find(new_dom.begin(), new_dom.end(),blk_name) == new_dom.end())
                new_dom.push_back(blk_name);

            // cout << "Final Intersection: ";
            // for (auto x : new_dom)
            //     cout << x << " ";
            // cout << endl;

            set<string> s1(dominators[blk_name].begin(), dominators[blk_name].end());
            set<string> s2(new_dom.begin(), new_dom.end());
            if (s1 != s2) {
                // cout << "!!!Change in Block: " << blk_name << endl;
                any_change = true;
                dominators[blk_name] = new_dom;
            }
        }
        if (any_change)
            change = true;
    }
    // cout << "===================End While======================\n\n";
    
    // cout << "==============DOMINATORS=======================" << endl;

    map<string,vector<string>> result;
    for (uint i = 0; i < all_blocks.size(); ++i) {
        string blk_name = all_blocks[i];

        // cout << "Block name: " << blk_name << " | "; 
        for (auto block : dominators[all_blocks[i]]) { 
            // cout << block << " ";
            result[block].push_back(blk_name);  
        }
        // cout << endl;
    }
    // cout << endl;
    // cout << "==========RESULT==========" << endl;
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        printf("@%s    ", blk->name);
        string name = blk->name;
        for (auto block : result[name]) {
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
