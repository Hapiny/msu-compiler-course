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
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <utility>
#include <set>


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
	int parent;
	std::vector<int> children;
	std::set<std::pair<Blk*, Blk*>> edges;

	Loop(Blk *h, Blk *t) : head(h), nodes(), parent(-1), children(){}
	void add_node(Blk *n){nodes.insert(n);}
};


struct Region{
	int head;
	std::string name;
	std::unordered_set<Blk*> blk_nodes;
	std::unordered_set<int> nodes;
	std::set<std::pair<int, int>> edges;
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

			if(is_subset_of(nodes, j_nodes))
				parents.insert(j);
		}

		int min_size = -1, parent_idx = -1;
		for(auto j : parents){
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


std::pair<std::vector<Region>, std::unordered_map<Blk*, int>> init_regions(Fn *fn){
	std::vector<Region> regions;
	std::unordered_map<Blk*, int> blk_map;

	for(Blk* b = fn->start; b; b = b->link){
		Region region;
		region.head = -1;
		region.name = b->name;
		region.name += "__r";
		region.blk_nodes.insert(b);
		blk_map[b] = regions.size();
		regions.push_back(region);
	}

	return std::make_pair(regions, blk_map);
}


void fill_edges(std::vector<Loop> &loops){
	for(Loop &l: loops){
		auto nodes = l.nodes;
		nodes.insert(l.head);

		for(Blk* node: nodes){
			if(node->s1 && nodes.find(node->s1) != nodes.end())
				l.edges.insert(std::make_pair(node, node->s1));

			if(node->s2 && nodes.find(node->s2) != nodes.end())
				l.edges.insert(std::make_pair(node, node->s2));
		}
	}
}


int find_region_for_blk(Blk* blk, std::vector<Region> &regions){
	for(int i = regions.size() - 1; i >= 0; i--){
		Region &reg = regions[i];

		if(reg.blk_nodes.find(blk) != reg.blk_nodes.end()){
			return i;
		}
	}

	return -1;
}



void process_loop(int idx, std::vector<Loop> &loops, std::vector<Region> &regions, std::unordered_map<Blk*, int> &blk_map, std::unordered_map<int, int> &loop_map){
	Loop &l = loops[idx];
	for(auto i:l.children)
		process_loop(i, loops, regions, blk_map, loop_map);

	std::unordered_set<int> loop_regions;
	
	std::unordered_set<Blk*> loop_nodes(l.nodes);
	auto loop_edges = l.edges;

	for(auto i:l.children){
		Loop &child = loops[i];
		
		loop_regions.insert(loop_map[i]);
		
		loop_nodes.erase(child.head);
		for(Blk* node:child.nodes)
			loop_nodes.erase(node);

		for(auto &edge : child.edges)
			loop_edges.erase(edge);
	}

	for(Blk* node : loop_nodes)
		loop_regions.insert(blk_map[node]);

	std::set<std::pair<int, int>> body_edges;

	for(auto& edge : loop_edges){
		Blk *from, *to;
		std::tie(from, to) = edge;

		int reg_from = find_region_for_blk(from, regions);
		int reg_to = find_region_for_blk(to, regions);

		if(to != l.head)
			body_edges.insert(std::make_pair(reg_from, reg_to));
	}

	Region body_region;
	body_region.head = find_region_for_blk(l.head, regions);
	loop_regions.erase(body_region.head);
	body_region.name = regions[body_region.head].name + "__r";
	body_region.edges = body_edges;
	body_region.nodes = loop_regions;
	body_region.blk_nodes = l.nodes;
	body_region.blk_nodes.insert(l.head);

	regions.push_back(body_region);
	
	int body_region_idx = regions.size() - 1;
	if(loop_regions.size() > 0){	
		Region loop_region;
		loop_region.head = body_region_idx;
		loop_region.name = regions[body_region.head].name + "__r" + "__r";
		loop_region.edges.insert(std::make_pair(body_region_idx, body_region_idx));
		loop_region.blk_nodes = l.nodes;
		loop_region.blk_nodes.insert(l.head);
		regions.push_back(loop_region);
	}
	else{
		regions[body_region_idx].edges.insert(std::make_pair(body_region.head, body_region.head));
	}

	loop_map[idx] = regions.size() - 1;
}

void process_final_region(Blk* start, std::vector<Region> &regions, std::vector<Loop> &loops, std::unordered_map<Blk*, int> &blk_map, std::unordered_map<int, int> &loop_map){
	std::set<std::pair<Blk*, Blk*>> edges_to_process;
	std::unordered_set<int> ovrl_regions;
	std::unordered_set<Blk*> blks_to_process;

	for(auto i: blk_map){
		Blk* blk = i.first;
		blks_to_process.insert(blk);
		if(blk->s1)
			edges_to_process.insert(std::make_pair(blk, blk->s1));
		if(blk->s2)
			edges_to_process.insert(std::make_pair(blk, blk->s2));
	}

	blks_to_process.erase(start);
	for(int i = 0; i < loops.size(); i++){
		Loop &l = loops[i];
		if(l.parent >= 0)
			continue;
		ovrl_regions.insert(loop_map[i]);
		for(Blk* b : l.nodes)
			blks_to_process.erase(b);
		blks_to_process.erase(l.head);
		for(auto& edge : l.edges)
			edges_to_process.erase(edge);
	}

	for(auto b: blks_to_process)
		ovrl_regions.insert(blk_map[b]);

	std::set<std::pair<int, int>> edges;

	for(auto& edge : edges_to_process){
		Blk *from, *to;
		std::tie(from, to) = edge;

		int reg_from = find_region_for_blk(from, regions);
		int reg_to = find_region_for_blk(to, regions);

		edges.insert(std::make_pair(reg_from, reg_to));
	}

	Region final_region;
	final_region.head = find_region_for_blk(start, regions);
	final_region.edges = edges;
	final_region.nodes = ovrl_regions;
	regions.push_back(final_region);
}



void print_regions(std::vector<Region>regions){
	for(auto& r : regions){
		if(r.head < 0){
			for(auto node: r.blk_nodes)
				printf("@%s : :\n", node->name);
			continue;
		}

		printf("@%s : ", regions[r.head].name.c_str());

		for(auto i : r.nodes)
			printf("@%s ", regions[i].name.c_str());

		printf(":");

		for(auto i : r.edges){
			Region &from = regions[i.first];
			Region &to = regions[i.second];
			printf(" @%s-@%s", from.name.c_str(), to.name.c_str());
		}
		printf("\n");
	}
}


void find_regions(Fn *fn){
	auto loops = build_loop_tree(fn);
	fill_edges(loops);

	std::vector<Region> regions;
	std::unordered_map<Blk*, int> blk_map;
	std::unordered_map<int, int> loop_map;
	std::tie(regions, blk_map) = init_regions(fn);

	for(int idx = 0; idx < loops.size(); idx++){
		if(loops[idx].parent >= 0)
			continue;
		process_loop(idx, loops, regions, blk_map, loop_map);
	}

	process_final_region(fn->start, regions, loops, blk_map, loop_map);
	print_regions(regions);
}


static void readfn (Fn *fn) {
	fillpreds(fn);
	fillrpo(fn);
	filluse(fn);
    ssa(fn);
    filluse(fn);
    find_regions(fn);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, (char *)"<stdin>", readdat, readfn);
  freeall();
}