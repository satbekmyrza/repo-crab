#ifndef DOMINANCE_HPP__
#define DOMINANCE_HPP__

#include <algorithm>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/dominator_tree.hpp>
#include <boost/unordered_map.hpp>

/*

  - A node u dominates v if all paths from entry to v pass through
    u.

  - A node u strictly dominates v if u dominates v and u!=v.

  - A node u is the immediate dominator of v (idom) if u is the unique
    node that strictly dominates v but does not strictly dominates any
    other node that strictly dominates v.

  - The dominance frontier of a node n is the set of nodes such that
    they are NOT strictly dominated by n but their predecessors are.

  The concepts of post-dominator and post-dominance frontier are dual
  and can be computed by computing dominators and dominance frontiers
  on the reverse graph.

*/

namespace crab {
  namespace analyzer {
   namespace graph_algo {

     using namespace std;
     using namespace boost;

     

     // Compute the dominator tree of a graph.
     // Out: idom is an associative container of pairs (u,v) where v
     //      is the immediate dominator of u. 
     template <typename G, typename Map>
     void dominator_tree(G g, typename G::node_t entry, Map &idom) {
       typedef typename G::node_t node_t;
       typedef typename graph_traits<G>::vertices_size_type vertices_size_type_t;       
       typedef associative_property_map<std::map <node_t, int> > index_map_t;
       typedef iterator_property_map<typename std::vector<vertices_size_type_t>::iterator,
				     index_map_t> time_map_t;
       typedef iterator_property_map<typename std::vector<node_t>::iterator,
				     index_map_t> pred_map_t;

       // build a map that maps graph vertices to indexes
       std::map <node_t, int> imap;
       index_map_t index_map (imap);
       typename graph_traits<G>::vertex_iterator It, Et;
       int j = 0;
       for (tie(It, Et) = vertices(g); It != Et; ++It, ++j) {
       	 put(index_map, *It, j);
       }

       std::vector<vertices_size_type_t> df_num(num_vertices (g), 0);
       time_map_t df_num_map(make_iterator_property_map(df_num.begin(), index_map));
       std::vector<node_t> parent(num_vertices(g),graph_traits<G>::null_vertex());
       pred_map_t parent_map(make_iterator_property_map(parent.begin(), index_map));
       std::vector<node_t> vertices_by_df_num(parent);
       std::vector<node_t> dom_tree_pred_vector(num_vertices(g), graph_traits<G>::null_vertex());
       pred_map_t dom_tree_pred_map(make_iterator_property_map(dom_tree_pred_vector.begin(),
							       index_map));
				    
       lengauer_tarjan_dominator_tree(g,entry ,
				      index_map, df_num_map, parent_map,
				      vertices_by_df_num, dom_tree_pred_map);


       for (auto v: boost::make_iterator_range (vertices (g))) {
	 if (get(dom_tree_pred_map, v) != graph_traits<G>::null_vertex()) {
	   idom.insert (typename Map::value_type (v, get(dom_tree_pred_map, v)));
	   CRAB_LOG ("dominator",
		     crab::outs () << crab::cfg_impl::get_label_str(get(dom_tree_pred_map, v))
		                   << " is the immediate dominator of "
 		                   << crab::cfg_impl::get_label_str(v)
		                   << "\n";); 
             		           
	 } else {
	   idom.insert (typename Map::value_type (v, graph_traits<G>::null_vertex()));
	   CRAB_LOG ("dominator",
		     crab::outs () << crab::cfg_impl::get_label_str(v)
		                   << " is not dominated by anyone!\n");
	 }
       }
     }

     namespace graph_algo_impl {       
       // OUT: df is a map from nodes to their dominance frontier
       template<typename G, typename VectorMap>
       void dominance (G g, typename G::node_t entry, VectorMap &df) {
	 typedef typename G::node_t node_t;
	 
	 // map node to its idom node
	 boost::unordered_map<node_t, node_t> idom;
	 dominator_tree (g, entry, idom);

	 // // map node n to all its immediate dominated nodes
	 // boost::unordered_map<node_t, std::vector<node_t> > dominated;
	 // for (auto v: boost::make_iterator_range (vertices (g))) {
	 //   if (idom[v] != graph_traits<G>::null_vertex()) {
	 //     auto &dom_vector = dominated[idom[v]];
	 //     dom_vector.push_back (v);
	 //   }
	 // }
	 
	 // computer dominance frontier
	 // use the iterative solution from Cooper/Torczon 
	 for (auto n: boost::make_iterator_range (vertices (g))) {
	   for (auto e: boost::make_iterator_range (in_edges(n,g))) {
	     // Traverse backwards the dominator tree starting from
	     // n's predecessors until found a node that immediate
	     // dominates n.
	     node_t runner = source (e, g);
	     while (runner != graph_traits<G>::null_vertex() &&
		    runner != idom[n] && runner != n) {	       
	       if (std::find(df[runner].begin(),df[runner].end(),n) == df[runner].end())
		 df[runner].push_back (n);
	       runner = idom[runner];
	     }
	   }
	 } // end outer for       

	 CRAB_LOG ("dominance",
		   for (auto &kv: df) {
		     crab::outs () <<  crab::cfg_impl::get_label_str(kv.first)
				   << "={";
		     for (auto v: kv.second) {
		       crab::outs () << crab::cfg_impl::get_label_str(v) << ";";
		     }
		     crab::outs () << "}\n";
		   });
       }
		 
     } // end namespace

     
     // OUT: a map that for each node returns its dominance frontier.     
     template<typename G, typename VectorMap>
     void dominance (G g, VectorMap &fdf) {
       CRAB_LOG("dominance", crab::outs () << "Dominance Frontiers\n");       
       graph_algo_impl::dominance (g, g.entry(), fdf);
     }

     // OUT: a map that for each node returns its post-dominance
     // (reverse/inverse)frontier.
     template<typename G, typename VectorMap>
     void post_dominance (G g, VectorMap &rdf) {
       if (!g.has_exit ()) return;
       cfg::cfg_rev<G> rev_g (g);
       CRAB_LOG("dominance", crab::outs () << "Post-Dominance Frontiers\n");
       graph_algo_impl::dominance (rev_g, rev_g.entry (), rdf);
       
     }
     
     
   } // end namespace
  } // end namespace
} // end namespace
#endif 
