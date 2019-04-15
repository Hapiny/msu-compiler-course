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
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <tuple>
#include <memory>
#include <iostream>
#include <algorithm>


template<typename UnorderedIn1, typename UnorderedIn2>
bool intersects(const  UnorderedIn1 &in1, const  UnorderedIn2 &in2){
    if (in2.size() < in1.size()) {
        return intersects<UnorderedIn2,UnorderedIn1>(in2, in1);
    }

    auto e = in2.end();
    for(auto & v : in1)
        if (in2.find(v) != e)
            return true;
    
    return false;
}

template<typename UnorderedIn1, typename UnorderedIn2>
bool is_subset_of(const std::unordered_set<UnorderedIn1>& a, const std::unordered_set<UnorderedIn2>& b){
    if (a.size() > b.size())
        return false;

    auto const not_found = b.end();
    for (auto const& element: a)
        if (b.find(element) == not_found)
            return false;

    return true;
}

struct Loop {
	Blk *head;
	std::unordered_set<Blk*> nodes;
	Blk *tail;
	Blk *prehead;
	int parent;
	std::vector<int> children;

	Loop(Blk *h, Blk *t) : head(h), nodes(), tail(t), parent(-1), children(), prehead(NULL) {}
	
	void add_node(Blk *n){nodes.insert(n);}
};

bool is_dominator(Blk* dom, Blk* target) {
	do {
		if(target == dom)
			return true;
	}
	while((target = target->idom));

	return false;
}


void fill_loop(Loop &l, Blk* node, std::unordered_set<Blk*> &visited){
	if(node == l.head)
		return;

	if(visited.find(node) != visited.end())
		return;

	visited.insert(node);
	l.add_node(node);
	
	for(uint i = 0; i < node->npred; i++)
		fill_loop(l, node->pred[i], visited);
}


std::vector<Loop> find_loops(Fn *fn) {
	std::vector<Loop> loops;

	for(auto b = fn->start; b; b = b->link) {
		for(auto succ :{b->s1, b->s2}) {
			if(!is_dominator(succ, b))
				continue;

			Loop l = Loop(succ, b);
			std::unordered_set<Blk*> visited; 
			fill_loop(l, b, visited);

			bool found_near_loop = false;

			for(uint i = 0; i < loops.size(); i++){
				if(loops[i].head == l.head && !is_subset_of(loops[i].nodes, l.nodes) && !is_subset_of(l.nodes, loops[i].nodes)){
					loops[i].nodes.insert(l.nodes.begin(), l.nodes.end());
					found_near_loop = true;
					break;				
				}
			}

			if(found_near_loop)
				continue;

			loops.push_back(l);
		}
	}

	return loops;
}

std::vector<Loop> build_loop_tree(Fn *fn){
	auto loops = find_loops(fn);
    for (auto loop : loops) {
        std::cout << loop.head->name << std::endl;
        std::cout << loop.parent << std::endl;
        std::cout << "===================" << std::endl;
    }
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	for(uint i = 0; i < loops.size(); i++){
		Loop &cur = loops[i];
		std::unordered_set<int> parents;
		std::unordered_set<Blk*> nodes(cur.nodes);
		nodes.insert(cur.head);

		for(uint j = 0; j < loops.size(); j++){
			if(i==j)
				continue;
			std::unordered_set<Blk*> j_nodes(loops[j].nodes);
			j_nodes.insert(loops[j].head);
            for (auto b : nodes)
                std::cout << b->name << std::endl;
            std::cout << std::endl;
            for (auto b : j_nodes)
                std::cout << b->name << std::endl;
            std::cout << "++++++++++++++++++++++++++++" << std::endl;
			if(is_subset_of(nodes, j_nodes)) {
				std::cout << i << " " << j << std::endl;
                parents.insert(j);
            }
		}

		int min_size = -1, parent_idx = -1;
		for(auto j : parents){
            std::cout << "____" << std::endl;
            std::cout << j << std::endl;
            std::cout << "____" << std::endl;
			if(min_size < 0 || loops[j].nodes.size() < min_size){
				parent_idx = j;
				min_size = loops[j].nodes.size();
			}
		}

		if(parent_idx >= 0){
			cur.parent = parent_idx;
			loops[parent_idx].children.push_back(i);
		}
	}

	return loops;
}

bool can_be_invariant(Ins &ins){
	uint op = ins.op;
	return !(isstore(op) || isload(op) || ispar(op) || op == Ocall || op == Ovacall || (op >= Oalloc4 && op < Ocopy));
}


void get_ins_to_process(Blk *b, std::vector<Ins*> &instructions){
	for(int i = 0; i < b->nins; i++)
		if(can_be_invariant(b->ins[i]))			
			instructions.push_back(&(b->ins[i]));
}


bool check_marked(uint tmp_val, std::vector<Ins*> &instructions, std::unordered_map<Ins*, bool> &m_ins){
	for(Ins *ins : instructions)
		if(ins->to.val == tmp_val)
			return m_ins[ins];

	return false;
}



bool is_invariant_ins(Ins &ins, Loop &l, Fn *fn, std::vector<Ins*> &instructions, std::unordered_map<Ins*, bool> &m_ins, std::unordered_map<int, Blk*> &var_defs){
	if(ins.arg[0].type == RCon and ins.arg[1].type == RCon)
		return true;

	uint tmp_0 = ins.arg[0].val, tmp_1 = ins.arg[1].val;
	Blk *def_block_0 = tmp_0 ? var_defs[tmp_0] : NULL, *def_block_1 = tmp_1 ? var_defs[tmp_1] : NULL;

	bool arg_0_invariant = !def_block_0 || check_marked(tmp_0, instructions, m_ins) || (def_block_0 != l.head && l.nodes.find(def_block_0) == l.nodes.end());
	bool arg_1_invariant = !def_block_1 || check_marked(tmp_1, instructions, m_ins) || (def_block_1 != l.head && l.nodes.find(def_block_1) == l.nodes.end());

	return arg_0_invariant && arg_1_invariant;
}

void delete_instructions(Blk* blk, std::vector<Ins*> &instructions, std::unordered_map<Ins*, bool> &m_ins){
	std::vector<Ins*> filtered;	

	for(int i = 0; i < blk->nins; i++)
		if(!m_ins[&(blk->ins[i])])
			filtered.push_back(&(blk->ins[i]));

	if(filtered.size() == blk->nins)
		return;

	Ins* new_instr = NULL;
	if(filtered.size() != 0){
		new_instr = new Ins[filtered.size()];
		int i = 0;
		for(Ins* ins : filtered){
			new_instr[i] = *ins;
			i += 1;
		}
	}

	// if(blk->nins > 0)
	// 	delete blk->ins;
	
	blk->ins = new_instr;
	blk->nins = filtered.size();
}


void process_loop(uint idx, std::vector<Loop> &loops, Fn *fn, std::unordered_map<int, Blk*> &var_defs){
	Loop &l = loops[idx];
	for(auto i:l.children)
		process_loop(i, loops, fn, var_defs);

	std::vector<Ins*> instructions;	
	get_ins_to_process(l.head, instructions);
	
	for(Blk* b:l.nodes){
		get_ins_to_process(b, instructions);
	}

	std::unordered_map<Ins*, bool> marked_ins;

	bool changed = true;
	while(changed){
		changed = false;

		for(Ins* ins: instructions){
			if(marked_ins[ins])
				continue;

			if(is_invariant_ins(*ins, l, fn, instructions, marked_ins, var_defs)){
				marked_ins[ins] = true;
				changed = true;
			}
		}
	}


	int count = 0;
	for(auto b:marked_ins)
		if(b.second)
			count++;

	if(!count)
		return;

	// printf("%s %d\n",l.head->name, count);

	Blk* prehead = new Blk;
	auto name_str = std::string("prehead@") + std::string(l.head->name);

	int i = 0;
	for(auto c: name_str){
		prehead->name[i] = c;
		i++;
	}

	prehead->link = l.head;
	prehead->ins = new Ins[count];
	prehead->nins = count;
	prehead->phi = NULL;
	prehead->s1 = l.head;
	prehead->jmp.type = Jjmp;

	int j = 0;
	for(Ins* ins: instructions){
		if(marked_ins[ins]){
			prehead->ins[j] = *ins;
			j++;
		}
	}
    std::cout << l.head->name << std::endl;
	delete_instructions(l.head, instructions, marked_ins);
	for(Blk* blk : l.nodes)
		delete_instructions(blk, instructions, marked_ins);

	if(l.parent >= 0)
		loops[l.parent].nodes.insert(prehead);

	l.prehead = prehead;
}


std::unordered_map<int, Blk*> get_variables_defs(Fn *fn){
	std::unordered_map<int, Blk*> res;

	for(Blk *b = fn->start; b; b = b->link){
		for(int i = 0; i < b->nins; i++)
			res[b->ins[i].to.val] = b;

		for(Phi *phi = b->phi; phi; phi = phi->link){
			res[phi->to.val] = b;
		}
	}

	return res;
}

void correct_preds(uint idx, std::vector<Loop> &loops, std::vector<Blk*> &blks){
	Loop &l = loops[idx];
	for(auto i:l.children)
		correct_preds(i, loops, blks);

	if(!l.prehead || !l.prehead->nins)
		return;

	for(Blk* b: blks){
		if(b == l.head){
			continue;		
		}

		if(b->s1 == l.head && l.nodes.find(b) == l.nodes.end())
			b->s1 = l.prehead;
		
		if(b->s2 == l.head && l.nodes.find(b) == l.nodes.end())
			b->s2 = l.prehead;
		
		if(b->link == l.head)
			b->link = l.prehead;
	}

	blks.push_back(l.prehead);
	// printf("\n");
}


void loop_motion(Fn *fn){
	auto loops = build_loop_tree(fn);
    for (auto loop : loops) {
        std::cout << loop.head->name << std::endl;
        std::cout << loop.parent << std::endl;
        std::cout << "===================" << std::endl;
    }
	auto var_defs = get_variables_defs(fn);

	std::vector<Blk*> blks;
	for(Blk* b = fn->start; b; b = b->link)
		blks.push_back(b);

	// printf("%d\n", loops.size());

	for(int idx = 0; idx < loops.size(); idx++){
		// printf("%s\n", loops[idx].head->name);
		// for(auto i: loops[idx].children)
		// 	printf("%s\t", loops[i].head->name);
		// printf("\n");
		// printf("\n");

		if(loops[idx].parent >= 0)
			continue;
		process_loop(idx, loops, fn, var_defs);
	}	

	// printf("correct preds\n");
	for(int idx = 0; idx < loops.size(); idx++){
		if(loops[idx].parent >= 0)
			continue;
		correct_preds(idx, loops, blks);
	}	
}

static void readfn (Fn *fn) {
	fillpreds(fn);
	fillrpo(fn);
	filluse(fn);
    ssa(fn);
    filluse(fn);
    // printfn(fn, stdout);
    loop_motion(fn);
    printfn(fn, stdout);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, (char *)"<stdin>", readdat, readfn);
  freeall();
}