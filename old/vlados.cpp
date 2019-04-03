#ifdef __cplusplus
#define export exports
extern "C" {
#include "../qbe/all.h"
}
#undef export
#else
#include <qbe/all.h>
#endif

#include <stdio.h>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cstring>
#include <memory>
#include <iostream>
#include <iterator>
#include <vector>
#include <utility>
#include <typeindex> 
#include <stack>

template<typename UnorderedIn1, typename UnorderedIn2,
         typename UnorderedOut = UnorderedIn1>
UnorderedOut makeIntersection(const  UnorderedIn1 &in1, const  UnorderedIn2 &in2)
{
    if (in2.size() < in1.size()) {
        return makeIntersection<UnorderedIn2,UnorderedIn1,UnorderedOut>(in2, in1);
    }

    UnorderedOut out;
    auto e = in2.end();
    for(auto & v : in1)
    {
        if (in2.find(v) != e){
            out.insert(v);
        }
    }
    return out;
}

template <class T>
inline void hash_combine(std::size_t & seed, const T & v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
 
namespace std
{
  template<typename S, typename T> struct hash<pair<S, T>>
  {
    inline size_t operator()(const pair<S, T> & v) const
    {
      size_t seed = 0;
      ::hash_combine(seed, v.first);
      ::hash_combine(seed, v.second);
      return seed;
    }
  };
}

struct BlockInfo{	
	char* name;
	Blk* blk_struct;
  	std::shared_ptr<BlockInfo> successer1;
  	std::shared_ptr<BlockInfo> successer2;
  	std::shared_ptr<BlockInfo> next_block;
  	std::unordered_set<std::shared_ptr<BlockInfo>> dominators;
  	std::unordered_set<std::shared_ptr<BlockInfo>> predeccesors;
  	std::shared_ptr<BlockInfo> idom;
  	std::unordered_set<std::shared_ptr<BlockInfo>> dominance_frontier;
  	std::unordered_set<std::shared_ptr<BlockInfo>> dominator_tree_successers;
  	std::unordered_set<std::string> def_set;
  	std::unordered_set<std::string> use_set;

  	BlockInfo(Blk* blk){
  		blk_struct = blk;
  		name = blk->name;
  		successer1 = nullptr;
  		successer2 = nullptr;
  		next_block = nullptr;
  		idom = nullptr;
  	}


	static std::unordered_map<std::string, std::shared_ptr<BlockInfo>> get_block_map(Fn *fn){

		std::unordered_map<std::string, std::shared_ptr<BlockInfo>> block_map;

		for (Blk *blk = fn->start; blk; blk = blk->link) {
			
			std::shared_ptr<BlockInfo> blk_info;
			auto find_succ = block_map.find(blk->name);

			if(find_succ == block_map.end()){
				
				blk_info = std::make_shared<BlockInfo>(blk);
				
				block_map.insert({blk->name, blk_info});
			}
			else{
				blk_info = find_succ->second;
			}
				
			if(blk->s1 != NULL)
			{
				auto find_succ = block_map.find(blk->s1->name);
				if(find_succ == block_map.end())
				{
					blk_info->successer1 = std::make_shared<BlockInfo>(blk->s1);
					block_map.insert({blk->s1->name, blk_info->successer1});
				}
				else
				{
					blk_info->successer1 = find_succ->second;
				}

				blk_info->successer1->predeccesors.insert(blk_info);
			}
			
			if(blk->s2 != NULL)
			{
				auto find_succ = block_map.find(blk->s2->name);
				if(find_succ == block_map.end())
				{
					blk_info->successer2 = std::make_shared<BlockInfo>(blk->s2);
					block_map.insert({blk->s2->name, blk_info->successer2});
				}
				else
				{
					blk_info->successer2 = find_succ->second;
				}
				blk_info->successer2->predeccesors.insert(blk_info);
			}
			
			if(blk->link != NULL)
			{
				auto find_succ = block_map.find(blk->link->name);
				if(find_succ == block_map.end())
				{
					blk_info->next_block = std::make_shared<BlockInfo>(blk->link);
					block_map.insert({blk->link->name, blk_info->next_block});
				}
				else
				{
					blk_info->next_block = find_succ->second;
				}
			}

			std::unordered_set<std::string> def_set;
		  	std::unordered_set<std::string> use_set;

		  	for(uint i = 0; i < blk->nins; i++){
		  		Ins ins = blk->ins[i];
		  		
		  		Ref to_variable = ins.to, arg1 = ins.arg[0], arg2 = ins.arg[1];
		  		
		  		char *to_name = fn->tmp[to_variable.val].name;
		  		char *arg1_name = fn->tmp[arg1.val].name;
		  		char *arg2_name = fn->tmp[arg2.val].name;
		  		int is_arg1_null = !strncmp(arg1_name, "", 1);
		  		int is_arg2_null =  !strncmp(arg2_name, "", 1);
		  		int is_to_null = !strncmp(to_name, "", 1);
		  		int is_to_variable = Tmp0 <= to_variable.val && to_variable.type == RTmp;
		  		int is_arg1_variable = Tmp0 <= arg1.val && arg1.type == RTmp;
		  		int is_arg2_variable = Tmp0 <= arg2.val && arg2.type == RTmp;

		  		if(is_arg1_variable && is_arg2_variable && is_arg1_null && is_arg2_null)
		  			if(is_to_variable && !is_to_null)
		  			{
		  				def_set.insert(to_name);
		  				continue;
		  			}

		  		if(is_arg1_variable && !is_arg1_null)
		  			if(def_set.find(arg1_name) == def_set.end()){
		  				use_set.insert(arg1_name);
		  			}

		  		if(is_arg2_variable && !is_arg2_null)
		  			if(def_set.find(arg2_name) == def_set.end()){
		  				use_set.insert(arg2_name);
		  			}

		  		if(is_to_variable && !is_to_null){
		  			def_set.insert(to_name);
		  		}
		  	}	
	  		if ((blk->s1 == NULL) && (blk->s2 == NULL)) {
	  			//its an exit block, check return
				char* ret_var_name = fn->tmp[blk->jmp.arg.val].name;
				int is_ret_null = !strncmp(ret_var_name, "", 1);
				if (!is_ret_null && def_set.find(ret_var_name) == def_set.end())
					use_set.insert(ret_var_name);
			}

			blk_info->use_set = use_set;
			blk_info->def_set = def_set;	
		}

		return block_map;
	}

	static void get_dominators(std::shared_ptr<BlockInfo> start_block){
		std::vector<std::shared_ptr<BlockInfo>> blocks;

		start_block->dominators.insert(start_block);

		for(auto i = start_block; i; i = i->next_block)
			blocks.push_back(i);

		for(auto i = start_block->next_block; i; i = i->next_block)
			i->dominators.insert(blocks.begin(), blocks.end());


		bool is_changed = true;

		while (is_changed){

			is_changed = false;

			for(uint i = 1; i < blocks.size(); i++){

				auto block = blocks[i];

				std::unordered_set<std::shared_ptr<BlockInfo>> new_dom, pred_intersection;
				bool first_flag = true;
				
				new_dom.insert(block);
				
				for(auto pred_block : block->predeccesors){
					if(first_flag)
					{
						pred_intersection = pred_block->dominators;
						first_flag = false;
					}
					else
						pred_intersection = makeIntersection(pred_intersection, pred_block->dominators);
				}

				new_dom.insert(pred_intersection.begin(), pred_intersection.end());

				if(new_dom != block->dominators)
				{
					block->dominators = new_dom;
					is_changed = true;
				}
			}
		}
	}

	static bool is_idom(std::shared_ptr<BlockInfo> i, std::shared_ptr<BlockInfo> n){
		if(i == n)
			return false;

		if(n->dominators.find(i) == n->dominators.end())
			return false;

		for(auto m:n->dominators)
			if((m != n) && (m != i) && (m->dominators.find(i) != m->dominators.end()))
				return false;

		return true;
	}

	static std::shared_ptr<BlockInfo> find_idom(std::shared_ptr<BlockInfo> block){
		for(auto dom : block->dominators){
			if(is_idom(dom, block))
				return dom;
		}

		return nullptr;
	}

	static void get_dominance_frontier(std::shared_ptr<BlockInfo> start_block){
		for(auto block = start_block; block; block = block->next_block){
			if(block->predeccesors.size() > 1){
				for(auto pred:block->predeccesors){
					auto r = pred;
					while(!is_idom(r, block)){
						r->dominance_frontier.insert(block);
						r = find_idom(r);
					}
				}
			}
		}
	}

	static void fill_dominator_tree_successers(std::shared_ptr<BlockInfo> start_block){
		std::vector<std::shared_ptr<BlockInfo>> blocks;

		for(auto i = start_block; i; i = i->next_block)
			blocks.push_back(i);

		for(auto i = start_block; i; i = i->next_block){
			for(auto j: blocks){
				if(is_idom(i, j))
					i->dominator_tree_successers.insert(j);
			}
		}	
	}

	static std::pair<std::unordered_set<std::string>, std::unordered_map<std::string, std::unordered_set<std::shared_ptr<BlockInfo>>>> 
	get_globals_and_blocks(std::shared_ptr<BlockInfo> start_block){
		std::unordered_set<std::string> globals;
		std::unordered_map<std::string, std::unordered_set<std::shared_ptr<BlockInfo>>> blocks;

		for(auto block = start_block; block; block = block->next_block){
			globals.insert(block->use_set.begin(), block->use_set.end());
			globals.insert(block->def_set.begin(), block->def_set.end());
		
			for(auto i: block->def_set)
				blocks[i].insert(block);
		}

		return std::make_pair(globals, blocks);
	}

};

struct MyPhi{
	std::string name;
	int index;
	std::unordered_set<std::pair<std::shared_ptr<BlockInfo>, int>> preds;
	MyPhi(std::string name): name(name), index(-1){}
};


std::unordered_map<std::shared_ptr<BlockInfo>, std::vector<MyPhi>> insert_phis(const std::unordered_set<std::string> &globals, 
	const std::unordered_map<std::string, std::unordered_set<std::shared_ptr<BlockInfo>>> &blocks){

	std::unordered_map<std::shared_ptr<BlockInfo>, std::vector<MyPhi>> phis;

	for(auto i: globals){
		std::unordered_set<std::shared_ptr<BlockInfo>> worklist = blocks.at(i);
		std::unordered_set<std::shared_ptr<BlockInfo>> marked;

		while(worklist.size() > 0){
			auto block = *(worklist.begin());
			worklist.erase(block);

			for(auto d: block->dominance_frontier){
				if(marked.find(d) == marked.end()){
					phis[d].push_back(MyPhi(i));
					worklist.insert(d);
					marked.insert(d);
				}
			}
		}
	}

	return phis;
}

int newname(std::string x, std::unordered_map<std::string, std::stack<int>> &stacks, std::unordered_map<std::string, int> &counters){
	int i = counters[x];
	counters[x]++;
	stacks[x].push(i);

	return i;
}

void rename(Fn *fn, std::shared_ptr<BlockInfo> block, std::unordered_map<std::string, std::stack<int>> &stacks, std::unordered_map<std::string, int> &counters, 
	std::unordered_map<std::shared_ptr<BlockInfo>, std::vector<MyPhi>> &phis){

	std::unordered_map<std::string, int> variable_indexes;

	for(MyPhi &phi :phis[block]){
		phi.index = newname(phi.name, stacks, counters);
		variable_indexes[phi.name] = phi.index;
	}

	for(uint i = 0; i < block->blk_struct->nins; i++){
  		Ins ins = block->blk_struct->ins[i];
  		Ref to_variable = ins.to;
  		
  		char *to_name = fn->tmp[to_variable.val].name;
  		int is_to_null = !strncmp(to_name, "", 1);
  		int is_to_variable = Tmp0 <= to_variable.val && to_variable.type == RTmp;
  		

  		if(is_to_variable && !is_to_null){
  			variable_indexes[to_name] = newname(to_name, stacks, counters);
  		}
	}	

	std::unordered_set<std::shared_ptr<BlockInfo>> successers;
	successers.insert(block->successer1);
	successers.insert(block->successer2);

	for(auto i: successers){
		if(!i)
			continue;

		for(MyPhi &phi :phis[i]){
			int index;
			if(stacks[phi.name].empty())
				index = -1;
			else
				index = stacks[phi.name].top();

			phi.preds.insert(std::make_pair(block, index));
		}
	}
	
	for(auto i: block->dominator_tree_successers)
		rename(fn, i, stacks, counters, phis);

	for(MyPhi &phi :phis[block])
		stacks[phi.name].pop();

	for(uint i = 0; i < block->blk_struct->nins; i++){
  		Ins ins = block->blk_struct->ins[i];
  		Ref to_variable = ins.to;
  		
  		char *to_name = fn->tmp[to_variable.val].name;
  		int is_to_null = !strncmp(to_name, "", 1);
  		int is_to_variable = Tmp0 <= to_variable.val && to_variable.type == RTmp;
  		

  		if(is_to_variable && !is_to_null){
  			stacks[to_name].pop();
  		}
	}	
}



static void readfn (Fn *fn) {

	auto block_map = BlockInfo::get_block_map(fn);

	auto start_block = block_map[fn->start->name]; 

	BlockInfo::get_dominators(start_block);

	BlockInfo::get_dominance_frontier(start_block);

	BlockInfo::fill_dominator_tree_successers(start_block);

	auto globals_blocks = BlockInfo::get_globals_and_blocks(start_block);
	auto globals = globals_blocks.first; auto blocks = globals_blocks.second;

	auto phis = insert_phis(globals, blocks);
	std::unordered_map<std::string, int> counters;
	std::unordered_map<std::string, std::stack<int>> stacks;

	// rename(fn, start_block, stacks, counters, phis);

	// for(auto i: phis){
	// 	printf("@%s:\n", i.first->name);

	// 	for(auto j: i.second){
	// 		printf("%%%s %d =", j.name.c_str(), j.index);
	// 		for(auto pr: j.preds)
	// 			printf(" @%s %d", pr.first->name, pr.second);
	// 		printf("\n");
	// 	}
	// 	printf("\n");
	// }
	for (auto var : globals) {
		std::cout << var << " ";
		// for (auto b_ptr : blocks[var]) 
		// 	std::cout << "@" << b_ptr->name << " ";
		// std::cout << std::endl; 
	}
	std::cout << std::endl;
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, (char *)"<stdin>", readdat, readfn);
  freeall();
}