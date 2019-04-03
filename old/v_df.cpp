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

};



static void readfn (Fn *fn) {

	auto block_map = BlockInfo::get_block_map(fn);

	auto start_block = block_map[fn->start->name]; 

	BlockInfo::get_dominators(start_block);

	BlockInfo::get_dominance_frontier(start_block);

	for(auto i = start_block; i; i = i->next_block)
	{
		printf("@%s:\t", i->name);

		for(auto j : i->dominance_frontier)
			printf("@%s ", j->name);		
		
		printf("\n");
	}
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, (char *)"<stdin>", readdat, readfn);
  freeall();
}