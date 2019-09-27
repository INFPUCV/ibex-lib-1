//============================================================================
//                                  I B E X
// File        : ibex_Optimizer.h
// Author      : Matias Campusano, Damir Aliquintui, Ignacio Araya
// Copyright   : IMT Atlantique (France)
// License     : See the LICENSE file
// Created     : Sep 24, 2017
// Last Update : Sep 24, 2017
//============================================================================

#ifndef __IBEX_OPTIMIZERMOP_H__
#define __IBEX_OPTIMIZERMOP_H__

#include "ibex_Ctc.h"
#include "ibex_Bsc.h"
#include "ibex_LoupFinderMOP.h"
#include "ibex_CtcKhunTucker.h"
#include "ibex_DistanceSortedCellBufferMOP.h"
#include "ibex_pyPlotter.h"
#include "ibex_PFunction.h"
#include "ibex_NDS.h"

#include <set>
#include <map>
#include <list>
#include <stack>
#include <math.h>
#include <iostream>
#include <unistd.h>

#include "ibex_BxpMOPData.h"
//#include "ibex_DistanceSorted.h"

using namespace std;
namespace ibex {

/**
 * comparation function for sorting NDS by decreasing y
 */
struct sorty{
	bool operator()(const pair<double,double> p1, const pair<double,double> p2){
		return p1.second>p2.second;
	}
};

static bool sort_using_smaller_than(double u, double v)
{
  return u < v;
}


static bool sort_using_middle_than(double u, double v)
{
  return fabs(u-0.5) < fabs(v-0.5);
}

/**
 * \brief Node_data
 *
 * This class is an assistant of the global optimization algorith
 *
 */


class Node_t{
public:

	/**
	 * \brief create the node
	 * 
	 * Inputs:
	 *	\param t 		   		   the interval of points contained in the node
	 *	\param dist 			   a
	 *
	 */
	
	Node_t(Interval t, double dist) : t(t), dist(dist) { }
	/**
	 * \brief compare 2 nodes based on the dist of both
	 */
	friend bool operator<(const Node_t& n1, const Node_t& n2){
		return n1.dist < n2.dist;
	}

	/* =========================== Settings ============================= */

	Interval t;
	set<double> b;
	double dist;
};


/**
 * \brief Global biObjetive Optimizer (ibexMOP).
 *
 * This class is an implementation of a global optimization algorithm for biObjective problems
 * described in https://github.com/INFPUCV/ibex-lib/tree/master/plugins/optim-mop by Araya et al.
 *
 * \remark In all the comments of this class, "NDS" means "Non Dominated Set"
 * related to the objectives f1 and f2
 */
class OptimizerMOP {

public:

	/**
	 * \brief Return status of the optimizer
	 *
	 * See comments for optimize(...) below.
	 */
	typedef enum {SUCCESS, INFEASIBLE, NO_FEASIBLE_FOUND, UNBOUNDED_OBJ, TIME_OUT, UNREACHED_PREC} Status;

	typedef enum {POINTS, SEGMENTS, HAMBURGER, /* splitting strategies */ MIDPOINT, MAXDIST, ALL} Mode;

  typedef enum {STAND_BY_FOCUS, STAND_BY_SEARCH, REACHED_PRECISION, SEARCH, FOCUS_SEARCH, FINISHED} ServerStatus;

	/**
	 *  \brief Create an optimizer.
	 *
	 * Inputs:
	 *   \param n        		   number of variables of the <b>original system</b>
	 *   \param f1	       		   the objective function f1
     *	 \param f2       		   the objective function f2
	 *   \param ctc       		   contractor for <b>extended<b> boxes (of size n+2)
	 *   \param bsc       		   bisector for <b>extended<b> boxes (of size n+2)
	 *   \param buffer    		   buffer for <b>extended<b> boxes (of size n+2)
	 *   \param finder    		   the finder of ub solutions
	 *   \param eps	      		   the required precision
	 *

	 *
	 * \warning The optimizer relies on the contractor \a ctc to contract the domain of the goal variable
	 *          and increase the uplo. If this contractor never contracts this goal variable,
	 *          the optimizer will only rely on the evaluation of f and will be very slow.
	 *
	 * We are assuming that the objective variables are n and n+1
	 *
	 */
	OptimizerMOP(int n, const Function &f1,  const Function &f2,
			Ctc& ctc, Bsc& bsc, CellBufferOptim& buffer, LoupFinderMOP& finder,
			Mode nds_mode=POINTS, Mode split_mode=MIDPOINT, double eps=default_eps, double rel_eps=0.0);

	/**
	 * \brief Delete *this.
	 */
	virtual ~OptimizerMOP();

	/**
	 * \brief Run the optimization.
	 *
	 * \param init_box             The initial box
	 *
	 * \return SUCCESS             if the global minimum (with respect to the precision required) has been found.
	 *                             In particular, at least one feasible point has been found, less than obj_init_bound, and in the
	 *                             time limit.
	 **
	 *         TIMEOUT             if time is out.
	 *
	 */
	Status optimize(const IntervalVector& init_box);

	/* =========================== Output ============================= */

	/**
	 * \brief Displays on standard output a report of the last call to optimize(...).
	 *
	 * Information provided:
	 * <ul><li> total running time
	 *     <li> totl number of cells (boxes)
	 *     <li> total number of best non-dominated solutions found
	 *     <li> the set of non-dominated solutions found
	 * </ul>
	 */
	void report(bool verbose=true);


	/**
	 * \brief Get the status.
	 *
	 * \return the status of last call to optimize(...).
	 */
	Status get_status() const;


	//std::set< point2 >& get_LB()  { return LB; }

	/**
	 * \brief Get the time spent.
	 *
	 * \return the total CPU time of last call to optimize(...)
	 */
	double get_time() const;

	/**
	 * \brief Get the number of cells.
	 *
	 * \return the number of cells generated by the last call to optimize(...).
	 */
	double get_nb_cells() const;

	/*
    * \brief Update the focus of solution
    *
    * This take in count the hull of the region and the found solutions
    *
    * Inputs:
    *    \param cells 				   a
    *    \param paused_cells		   a
    *    \param focus 				   a
    */

	void update_focus(set<Cell*>& cells, set<Cell*>& paused_cells, IntervalVector& focus){

		IntervalVector new_focus(2);
		new_focus.set_empty();

		for(auto cc:cells){
			IntervalVector boxy=get_boxy(cc->box,n);
			if(new_focus.is_empty())
				new_focus=boxy;
			else new_focus|=boxy;
		}

		for(auto cc:paused_cells){
			IntervalVector boxy=get_boxy(cc->box,n);
			if(new_focus.is_empty())
				new_focus=boxy;
			else new_focus|=boxy;
		}

		focus&=new_focus;

	}

	/*
    * \brief 
    *
    *
    * Inputs:
    *    \param cells 				   a
    *    \param paused_cells		   a
    *    \param focus 				   a
    */

  	void write_status(double rel_prec){
		ofstream output;
		output.open( (output_file+".state").c_str());
		switch(sstatus){
			case STAND_BY_SEARCH:
			case STAND_BY_FOCUS: output << "STAND_BY" ; break;
			case REACHED_PRECISION: output << "REACHED_PRECISION" ; break;
			case SEARCH: output << "SEARCH" ; break;
			case FOCUS_SEARCH: output << "FOCUS_SEARCH" ; break;
			case FINISHED: output << "FINISHED" ; break;
		}
		output << "," << rel_prec*100 << endl;
		output.close();
	}

	/*
    * \brief Print the region of solutions
    *
    * 
    *
    * Inputs:
    *    \param cells 				   a
    *    \param paused_cells		   a
    *    \param focus 				   a
    */

 	void write_envelope(set<Cell*>& cells, set<Cell*>& paused_cells, IntervalVector& focus){
		//escritura de archivos
		//dormir 1 segundo y lectura de instrucciones
		cout << "escritura de archivo" << endl;

		NDS_seg LBaux;
		NDS_seg UBaux=ndsH;


		update_focus(cells, paused_cells, focus);

		/*
		Vector v(2); v[0]=focus[0].lb(); v[1]=focus[1].ub();
		LBaux.addPoint(v);
		UBaux.addPoint(v);
		v[0]=focus[0].ub(); v[1]=focus[1].lb();
		LBaux.addPoint(v);
		UBaux.addPoint(v);
		*/

		for(auto cc:cells)	LBaux.add_lb(*cc);
		for(auto cc:paused_cells) LBaux.add_lb(*cc);

		//se escribe el archivo de salida
		IntervalVector focus2(2);
		focus2[0]=BxpMOPData::y1_init;
		focus2[1]=BxpMOPData::y2_init;
		update_focus(cells, paused_cells, focus2);

		py_Plotter::offline_plot(UBaux.NDS2,  &LBaux.NDS2, output_file.c_str(), &focus2);
	}

	/*
    * \brief Read the instruccions of work
    *
    * after reading a instruction, this delete it from the file
    *
    * Inputs:
    *    \param cells 				   a
    *    \param paused_cells		   a
    *    \param focus 				   a
    */

	void read_instructions(set<Cell*>& cells, set<Cell*>& paused_cells, IntervalVector& focus){


		//se lee el archivo de instrucciones y se elimina
		string line; ifstream myfile;
		myfile.open(instructions_file);
		if (myfile.is_open()){
			string instruction;
			myfile >> instruction ;
			if(instruction=="zoom_in" || instruction=="zoom_out"){
        if(instruction=="zoom_in") sstatus=FOCUS_SEARCH;
				if(instruction=="zoom_out") sstatus=SEARCH;

				double y1_lb,y1_ub,y2_lb,y2_ub;
				myfile >> y1_lb >> y1_ub;
				myfile >> y2_lb >> y2_ub;
				focus[0] = Interval(y1_lb,y1_ub);
				focus[1] = Interval(y2_lb,y2_ub);
				if(instruction == "zoom_out")
				{		focus[0]=BxpMOPData::y1_init;
						focus[1]=BxpMOPData::y2_init;
						update_focus(cells, paused_cells, focus);
				}

				cout << instruction << focus << endl;
				if(instruction=="zoom_out" || instruction=="zoom_in"){
					for(auto cc:paused_cells){
						buffer.push(cc);
						cells.insert(cc);
					}
					paused_cells.clear();
					max_dist_eps=NEG_INFINITY;
				}

			}else if(instruction=="get_solution"){
				string output_file;
				double y1,y2;
				myfile >> output_file;
				myfile >> y1 >> y2;


				Vector y(2); y[0]=y1; y[1]=y2;

				pair<Vector, NDS_data> data = ndsH.get(y);
				cout << "y:" << data.first << endl;
				if(data.second.x1) cout << *data.second.x1 << endl;
				if(data.second.x2) cout << *data.second.x2 << endl;

				Vector* v=NULL;
				Vector realy(2);
				if(!data.second.x2 || data.second.x1 == data.second.x2){
					realy[0]=eval_goal(goal1, *data.second.x1, data.second.x1->size()).ub();
					realy[1]=eval_goal(goal2, *data.second.x1, data.second.x1->size()).ub();
					if( realy[0] < y[0] + eps && realy[1] < y[1] + eps)
					  v=new Vector(*data.second.x1);
				}else{
					PFunction pf(goal1, goal2, *data.second.x1, *data.second.x2);
					v=pf.find_feasible(y, 1e-8);
				}

				ofstream output, output_tmp;
				output.open(output_file);
				output_tmp.open("output.tmp");
				if(v){
					realy[0]=eval_goal(goal1, *v, v->size()).ub();
					realy[1]=eval_goal(goal2, *v, v->size()).ub();
					output << *v << endl;
					output << realy << endl;
					output_tmp << *v << endl;
					output_tmp << realy << endl;
					delete v;
				}else {
					output << "not found" << endl;
					output_tmp << "not found" << endl;
				}
				output.close();
				output_tmp.close();

			}else if(instruction=="pause"){
				 if(sstatus==SEARCH) sstatus=STAND_BY_SEARCH;
         else if(sstatus=FOCUS_SEARCH) sstatus=STAND_BY_FOCUS;
			 }else if(instruction=="continue"){
				 if(sstatus==STAND_BY_SEARCH) sstatus=SEARCH;
         else if(sstatus=STAND_BY_FOCUS) sstatus=FOCUS_SEARCH;
			}else if(instruction=="finish"){
				 sstatus=FINISHED;
			}

			myfile.close();
			rename(instructions_file.c_str(), (instructions_file+".old").c_str());




		}

	}

	/*
    * \brief Get the box of potential solutions
    *
    * Inputs:
    *    \param v 				       a
    *    \param n		               a
    */

	static IntervalVector get_boxy(IntervalVector& v, int n){
		IntervalVector boxy(2);
		boxy[0]=v[n];
		boxy[1]=v[n+1];
		return boxy;
	}

	/* =========================== Settings ============================= */

	/**
	 * \brief Number of variables.
	 */
	const int n;

	/**
	 * \brief Objective functions
	 * Functions have the form: f1 - z1  and f2 - z2. Thus, in order to
	 * evaluate them we have to set z1 and z2 to [0,0].
	 */
	const Function& goal1;
	const Function& goal2;

	/**
	 * \brief Contractor for the extended system.
	 *
	 * The extended system:
	 * (y=f(x), g_1(x)<=0,...,g_m(x)<=0).
	 */
	Ctc& ctc;

	/**
	 * \brief Bisector.
	 *
	 * Must work on extended boxes.
	 */
	Bsc& bsc;

	/**
	 * Cell buffer.
	 */
	CellBuffer& buffer;

	/**
	 * \brief LoupFinder
	 */
	LoupFinderMOP& finder;

	/** Required precision for the envelope */
	double eps;

	/** Required relative precision for the envelope */
	double rel_eps;


	/** Default precision: 0.01 */
	static const double default_eps;


  //server variables
	static bool _server_mode;
	static string instructions_file;
	static string output_file;

	

	/**
	 * \brief Trace activation flag.
	 */
	int trace;

	/**
	 * \brief Time limit.
	 *
	 * Maximum CPU time used by the strategy.
	 * This parameter allows to bound time consumption.
	 * The value can be fixed by the user.
	 */
	double timeout;

	//pair <double, double> y1ref;
	//pair <double, double> y2ref;

	//Interval y1refi;
	//Interval y2refi;

	/* ======== Some other parameters of the solver =========== */

	//if true: Save a file to be plotted by plot.py (default value= false).
	bool static _plot;

	//Min distance between two non dominated points to be considered, expressed as a fraction of eps (default value= 0.1)
	double static _min_ub_dist;

	//True: the solver uses the upper envelope of the cy contract for contraction
	static bool _cy_upper;

	//True: the solver uses the lower envelope of the cy contract in contraction
	static bool cy_contract_var;

	//True: the solver reduces the search spaces by reducing the NDS vectors in (eps, eps)
	static bool _eps_contract;

	//Termination criteria for the hamburger algorithm (dist < rh*ini_dist)
	static double _rh;

  //max distance of cells rejected by eps-close
  double max_dist_eps;

	//NDS mode: POINTS or SEGMENTS
	Mode nds_mode;
	Mode split_mode;


	/**
	 * \brief Evaluate the goal in the point x
	 */
	static Interval eval_goal(const Function& goal, const IntervalVector& x, int n);

	/**
	 * \brief Gradient of the goal in the point x
	 */
	static IntervalVector deriv_goal(const Function& goal, const IntervalVector& x, int n);

	/*double distance(const Cell* c){
		return NDS_seg::distance(c);
	}*/

	// Hamburger
	NDS_seg ndsH;

protected:

	/**
	 * \brief Hamburger Algorithm
	 */
	void hamburger(const IntervalVector& aIV, const IntervalVector& bIV);

   bool process_node(PFunction& pf, Node_t& n_t);

	void cy_contract2(Cell& c, list < Vector >& inpoints);

	
	/**
	 * \brief return the first and last points dominating the lb of the box
	 */
	static list<pair <double,double> > extremal_non_dominated(const IntervalVector& box);



	/**
	 * \brief Contract and bound procedure for processing a box.
	 *
	 * <ul>
	 * <li> contract the cell using #dominance_peeler() and #discard_generalized_monotonicty_test(),
	 * <li> contract with the contractor ctc,
	 * </ul>
	 *
	 */
	void contract_and_bound(Cell& c, const IntervalVector& init_box);


	/**
	 * \brief The box is reduced using NDS2
	 *
	 * Adaptation of dominance_peeler to NDS2
	 * Returns the set of non-dominated segments in box
	 */
	void dominance_peeler2(IntervalVector &box, list < Vector >& inpoints);

    /**
     *  \brief returns true if the facet orthogonal to the i direction of the box is feasible.
     *
     *  See #discard_generalized_monotonicty_test()
     */
    bool is_inner_facet(IntervalVector box, int i, Interval bound){
    	box.resize(n);
    	box[i]=bound;
    	return finder.norm_sys.is_inner(box);
    }


    /**
     * \brief Implements the method for discarding boxes proposed in [J. Fernandez and B. Toth,
     * "Obtaining the efficient set of nonlinear biobjective optimiza-tion  problems  via  interval
     * branch-and-bound  methods" (2009)]
     */
	void discard_generalized_monotonicty_test(IntervalVector& box, const IntervalVector& initbox);



	/**
	 * \brief Main procedure for updating the NDS.
	 * <ul>
	 * <li> finds two points xa and xb in a polytope using the #LoupFinderMOP,
	 * <li> finds a segment passing over the points (f1(x),f2(x)) in the segment xa-xb,
	 * <li> add the segment to NDS
	 * </ul>
	 */
	bool update_NDS2(const IntervalVector& box);



private:

	/* ======== Some other parameters of the solver =========== */

	/**
	 * min feasible value found for each objective
	 */
    pair <double, double> y1_ub, y2_ub;

	/* Remember return status of the last optimization. */
	Status status;

	ServerStatus sstatus;


	/** The current non-dominated set sorted by increasing x */
	//static map< pair <double, double>, IntervalVector, sorty2 > NDS2;

	/* CPU running time of the current optimization. */
	double time;

	/** Number of cells pushed into the heap (which passed through the contractors) */
	int nb_cells;

};

inline OptimizerMOP::Status OptimizerMOP::get_status() const { return status; }

inline double OptimizerMOP::get_time() const { return time; }

inline double OptimizerMOP::get_nb_cells() const { return nb_cells; }




} // end namespace ibex

#endif // __IBEX_OPTIMIZER_H__
