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
#include <tuple>

using namespace std;

template<typename T>
bool is_in(T blk, set<T> blks) {
    if (blks.find(blk) != blks.end()) {
        return true;
    }
    return false;
}

class Loop {
public:
    Loop(Blk*); 
    
    Blk* loop_start = nullptr;
    Blk* loop_header = nullptr;
    set<Blk*> blks = {};
    
    // if this loop inside another loop
    int outer_loop_id = -1;
    // if this loop has another loops inside
    vector<int> inside_loop_ids;
    set<tuple<Blk*, Blk*>> loop_edge = {};   
};

Loop::Loop(Blk* s): loop_start(s) {}

class Region{
public:
    Region(string);
	
    string region_name = "";
	int region_index = -1;
	set<Blk*> blks = {};
	set<int> inside_regions_ids = {};
	set<tuple<int, int>> region_edges = {};
};

Region::Region(string name): region_name(name) {}

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

int blk2region_index(Blk* blk, vector<Region> &regions){
	for(int i = regions.size() - 1; i >= 0; i--) {
		if (is_in(blk, regions[i].blks)) {
            return i;
        }
	}
	return -1;
}

// copy from licm.cpp
void loop_blks_fill(Loop &loop, Blk* blk, set<Blk*> &visited){
	if(blk == loop.loop_start || visited.find(blk) != visited.end())
		return;
    else {
        loop.blks.insert(blk);
        visited.insert(blk);
        for(uint i = 0; i < blk->npred; i++)
            loop_blks_fill(loop, blk->pred[i], visited);
    }
}
// copy from licm.cpp
void fill_all_loops(Fn *fn, vector<Loop> &loops) {
	for(Blk* blk = fn->start; blk; blk = blk->link) {
        vector<Blk*> successors = {};
        successors.push_back(blk->s1);
        successors.push_back(blk->s2);
		for(auto s : successors) {
			if(is_dominator(blk, s)) {
                Loop loop = Loop(s);
                bool flag = false;
                set<Blk*> visited; 
                loop_blks_fill(loop, blk, visited);

                for(uint i = 0; i < loops.size(); i++){
                    if (
                        loops[i].loop_start == loop.loop_start
                        && !includes(loops[i].blks.begin(), loops[i].blks.end(), loop.blks.begin(), loop.blks.end())
                        && !includes(loop.blks.begin(), loop.blks.end(), loops[i].blks.begin(), loops[i].blks.end()) 
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
//copy from licm.cpp
vector<Loop> fill_loops(Fn *fn) {
    // fill blks for each loop
    vector<Loop> loops;
	fill_all_loops(fn, loops);
    // for (auto loop : loops) {
    //     cout << loop.loop_start->name << endl;
    //     cout << loop.outer_loop_id << endl;
    //     cout << "===================" << endl;
    // }
    // cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	for(uint i = 0; i < loops.size(); i++){
		Loop& loop = loops[i];
		set<int> parents;
		set<Blk*> blocks(loop.blks);
		blocks.insert(loop.loop_start);

		for(uint j = 0; j < loops.size(); j++){
			if (i != j) {
                set<Blk*> another_blks(loops[j].blks);
                another_blks.insert(loops[j].loop_start);

                // for (auto b : blocks)
                //     cout << b->name << endl;
                // cout << endl;
                // for (auto b : another_blks)
                //     cout << b->name << endl;
                // cout << "++++++++++++++++++++++++++++" << endl;
                if (includes(another_blks.begin(), another_blks.end(), blocks.begin(), blocks.end())) {
                    // cout << i << " " << j << endl;
                    parents.insert(j);
                }
            }
		}
		int size = -1; 
        int outer_loop_id = -1;
		for(int p_idx : parents){
            // cout << "____" << endl;
            // cout << p_idx << endl;
            // cout << "____" << endl;
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
    for (uint i = 0; i < loops.size(); i++) {
        set<Blk*> loop_blks = loops[i].blks;
        loop_blks.insert(loops[i].loop_start);
        for (auto blk : loop_blks) {
            vector<Blk*> successors = {};
            successors.push_back(blk->s1);
            successors.push_back(blk->s2);
            for (Blk* s : successors) {
                if (s != nullptr and is_in(s , loop_blks)) {
                    loops[i].loop_edge.insert(make_tuple(blk, s));
                }
            }
        }
    }
	return loops;
}

void fill_region_loops(int loop_index, vector<Loop>& loops,  unordered_map<int, int>& loop2loop,
                  unordered_map<Blk*, int>& block2number, Fn* fn,
                  vector<Region>& all_regions) {

    // for (auto p : loop2loop) {
	// 	cout << p.first << " " << endl;
	// }                      
    // cout << "------------------" << endl;
	Loop& loop = loops[loop_index];
	
    for (int i : loop.inside_loop_ids) {
		fill_region_loops(i, loops, loop2loop, block2number, fn, all_regions);
    }

    set<int> new_region_loops;
	set<Blk*> loop_blks = loop.blks;
	set<tuple<Blk*, Blk*>> loop_edges = loop.loop_edge;
	set<tuple<int, int>> body_edges;

	for (int inside_loop_id : loop.inside_loop_ids) {
		new_region_loops.insert(loop2loop[inside_loop_id]);
		loop_blks.erase(loops[inside_loop_id].loop_start);
		for(Blk* blk: loops[inside_loop_id].blks) {
			loop_blks.erase(blk);
        }
		for(tuple<Blk*, Blk*> edge : loops[inside_loop_id].loop_edge) {
			loop_edges.erase(edge);
        }
	}
	for (Blk* blk : loop_blks) {
		new_region_loops.insert(block2number[blk]);
    }
	for (tuple<Blk*, Blk*> edge : loop_edges) {
		if(get<1>(edge) != loop.loop_start) {
			int region_1 = blk2region_index(get<0>(edge), all_regions);
            int region_2 = blk2region_index(get<1>(edge), all_regions);
            body_edges.insert(make_tuple(region_1, region_2));
        }
	}
    // wrap region loop with edges to big region
	int body_region_idx = all_regions.size();
	int big_region_index = blk2region_index(loop.loop_start, all_regions);
	Region body_region(all_regions[big_region_index].region_name + "__r");
    
    body_region.region_index = big_region_index;
	body_region.region_edges = body_edges;
	new_region_loops.erase(body_region.region_index);
	body_region.inside_regions_ids = new_region_loops;
	body_region.blks = loop.blks;
	body_region.blks.insert(loop.loop_start);
	all_regions.push_back(body_region);

	if (new_region_loops.size()) {	
		Region loop_region(all_regions[body_region.region_index].region_name + "__r__r");

		loop_region.region_index = body_region_idx;
		loop_region.region_edges.insert(make_tuple(body_region_idx, body_region_idx));
		loop_region.blks = loop.blks;
		loop_region.blks.insert(loop.loop_start);
        
		all_regions.push_back(loop_region);
	} else {
		all_regions[body_region_idx].region_edges.insert(make_tuple(body_region.region_index, body_region.region_index));
	}
	loop2loop[loop_index] = all_regions.size() - 1;
}

void wrap_to_one_region(Blk* blk, vector<Loop>& loops, unordered_map<int, int> &loop2loop,
                        vector<Region>& all_regions, unordered_map<Blk*, int>& block2number) 
{
	set<int> body_regions;
	set<tuple<Blk*, Blk*>> all_edges;
	set<Blk*> all_blks;
	for (auto p : block2number) {
		all_blks.insert(p.first);
        vector<Blk*> successors = {};
        successors.push_back(p.first->s1);
        successors.push_back(p.first->s2);
        for (Blk* s : successors) {
            if (s) {
                all_edges.insert(make_tuple(p.first, s));
            }
        }
    }
	all_blks.erase(blk);
	for (int i = 0; i < loops.size(); i++) {
		if(loops[i].outer_loop_id == -1) {
            body_regions.insert(loop2loop[i]);
            for (Blk* b : loops[i].blks) {
                all_blks.erase(b);
            }
            all_blks.erase(loops[i].loop_start);
            for (tuple<Blk*, Blk*> edge : loops[i].loop_edge) {
                all_edges.erase(edge);
            }
        }
	}
	for(auto b: all_blks) {
		body_regions.insert(block2number[b]);
    }
	set<tuple<int, int>> edges_beetween_regions;
    for (tuple<Blk*, Blk*> edge : all_edges) {
        int region_1 = blk2region_index(get<0>(edge), all_regions);
        int region_2 = blk2region_index(get<1>(edge), all_regions);
        edges_beetween_regions.insert(make_tuple(region_1, region_2));
	}
    // add region which wrappes whole control flow graph
	Region whole_cfg("big_region");
	whole_cfg.region_index = blk2region_index(blk, all_regions);
	whole_cfg.region_edges = edges_beetween_regions;
	whole_cfg.inside_regions_ids = body_regions;
	all_regions.push_back(whole_cfg);
}

static void readfn (Fn *fn) {
    fillrpo(fn);
    fillpreds(fn);
    filluse(fn);
    ssa(fn);

    // collect all variable defs
	unordered_map<Blk*, int> block2number;
    unordered_map<int, int> loop_index2loop_index;
    vector<Blk*> all_blocks = {};
    // collect blocks as region-leafs
    vector<Region> all_regions= {};
    int num = 0;
	for(Blk* blk = fn->start; blk; blk = blk->link) {
        Region region_leaf = Region(string(blk->name) + "__r");
        region_leaf.blks.insert(blk);
        block2number[blk] = num;
        all_blocks.push_back(blk);
        all_regions.push_back(region_leaf);
        num += 1;
	}
    // for (auto r : block2number) {
    //     cout << r.first->name << " " << r.second << endl;
    // }

    vector<Loop> all_loops = fill_loops(fn);
    // for (Loop l : all_loops) {
    //     cout << l.loop_start->name << endl;
        
    //     cout << "EDGES:" << endl;
    //     for (auto edge : l.loop_edge) {
    //         cout << get<0>(edge)->name << " -> " << get<1>(edge)->name << endl;
    //     }
    //     cout << "=========" << endl;

    // }
    // cout << "LOOPS SIZE : " << all_loops.size() << endl;

    for(int i = 0; i < all_loops.size(); i++) {
        // if this loop is outer create preheader
		if(all_loops[i].outer_loop_id == -1) {
            fill_region_loops(i, all_loops, loop_index2loop_index, block2number, fn, all_regions);
        }
	}
    // cout << "REGION SIZE : " << all_regions.size() << endl;
    wrap_to_one_region(fn->start, all_loops, loop_index2loop_index, all_regions, block2number);
    for(int i = 0; i < all_regions.size(); i++) {
        Region region = all_regions[i];
        if (all_regions[i].region_index == -1) {
            for (Blk* blk : all_regions[i].blks) {
                cout << "@" << blk->name << " : :" << endl;
            }
        } else {
            cout << "@" << all_regions[region.region_index].region_name << " : ";
            for (int inside_reg_id : region.inside_regions_ids) {
                cout << "@" << all_regions[inside_reg_id].region_name << " ";
            }
            cout << ": ";
            for (tuple<int, int> region_edge : region.region_edges) {
                cout << "@" << all_regions[get<0>(region_edge)].region_name 
                     << "-@" << all_regions[get<1>(region_edge)].region_name << " ";
            }
            cout << endl;
        }
    }
    // printfn(fn, stdout);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, (char *)"<stdin>", readdat, readfn);
  freeall();
}