/*
 * File:   ltlfchecker.h
 * Author: Jianwen Li
 * Note: SAT-based LTLf satisfiability checking
 * Created on June 26, 2017
 */

#ifndef LTLF_CHECKER_H
#define LTLF_CHECKER_H

#include <functional>
#include <map>
#include <utility>
#include <vector>

#include "carsolver.h"
#include "debug.h"
#include "evidence.h"
#include "formula/aalta_formula.h"
#include "solver.h"

namespace aalta {

struct MUSInfo {
  std::vector<int> mus;
  double           checker_creation_time;
  double           checker_check_time;
  double           mus_extraction_time;

  MUSInfo(const std::vector<int>& m, double create_time, double check_time, double extract_time)
      : mus(m),
        checker_creation_time(create_time),
        checker_check_time(check_time),
        mus_extraction_time(extract_time) {}
};

class AaltaSolver;
class LTLfChecker {
public:
  LTLfChecker() {};
  LTLfChecker(aalta_formula* f, bool verbose = false, bool evidence = false)
      : to_check_(f), verbose_(verbose) {
    solver_ = new CARSolver(f, verbose);
    if (evidence)
      evidence_ = new Evidence();
    else
      evidence_ = NULL;
  }

  bool add_assumptions(std::vector<aalta_formula*>& ass) {
    return solver_->add_assumptions(ass);
  }

  void create_solver() {}
  ~LTLfChecker() {
    if (solver_ != NULL)
      delete solver_;
    if (evidence_ != NULL)
      delete evidence_;
  }
  bool                 check();
  void                 print_evidence();
  void                 print_uc();
  unsigned int         get_uc_size();
  void                 print_mus();
  void                 print_all_mus();
  unsigned int         get_mus_size();
  std::vector<MUSInfo> enumerate_all_mus_v2(std::vector<aalta_formula*>& formulas,
                                            double                       first_creation_time,
                                            double                       first_check_time);
  std::vector<std::vector<int>> get_temporal_mus();

protected:
  // flags
  bool       verbose_;  // default is false
  CARSolver* solver_;   // SAT solver for computing next states
  // Note: Currently we only use one solver

  std::vector<aalta_formula*> names_;
  AaltaSolver*                bool_solver_;
  std::vector<int>            external_assumptions_;
  void                        print_mus(const std::vector<int>& mus);
  typedef std::
      map<int, aalta_formula*, std::less<int>, std::allocator<std::pair<const int, aalta_formula*>>>
              formula_map;
  formula_map ext_assumptions_map_;

  std::vector<int> extract_single_mus(aalta_formula*          gamma,
                                      const std::vector<int>& ext_assumptions);
  bool             block_up(const std::vector<int>& mus);
  bool             block_down(const std::vector<int>& assumptions);
  std::vector<int> get_model_assumptions();
  bool             solve_with_assumptions(const std::vector<int>& assumptions);
  void             initialize_bool_solver(const std::vector<int>& ext_assumptions);
  std::vector<int> get_external_assumptions(aalta_formula* gamma);
  std::vector<int> get_all_assumption_vars();
  std::vector<int> get_all_assumptions();

  enum RES { UNKNOW = -1, UNSAT, SAT };
  aalta_formula* to_check_;
  // visited_/traces_ is updated during the search process.
  // If the checking result is SAT, traces_ finally store a counterexample
  std::vector<aalta_formula*> visited_;
  Evidence*                   evidence_;
  unsigned int                uc_size_  = 0;
  unsigned int                mus_size_ = 0;

  //////////functions
  bool sat_once(aalta_formula* f);  // check whether the formula can be satisfied in one step (the
                                    // terminating condition of checking)
  RES         check_with_heuristics();
  bool        contain_global(aalta_formula*);
  bool        global_part_unsat(aalta_formula*);
  bool        dfs_check(aalta_formula* f);
  Transition* get_one_transition_from(aalta_formula*);
  void        push_formula_to_explored(aalta_formula* f);
  void        push_uc_to_explored();
  // check the satisfiability via olg_formula heuristics
  RES olg_sat(aalta_formula* f, bool keep_evidence);
  // check the unsatisfiability via olg_formula heuristics
  RES olg_unsat(aalta_formula* f, bool keep_evidence);

  void print_formulas_id(aalta_formula*);

  inline bool detect_unsat() {
    return solver_->unsat_forever();
  }
};

}  // namespace aalta

#endif
