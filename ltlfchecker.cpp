/*
 * File:   ltlfchecker.cpp
 * Author: Jianwen Li
 * Note: SAT-based LTLf satisfiability checking
 * Created on June 26, 2017
 */

#include "ltlfchecker.h"

#include <iostream>
#include <set>
#include <vector>

#include "aaltasolver.h"
#include "carchecker.h"
#include "formula/olg_formula.h"

using namespace std;

namespace aalta {
bool LTLfChecker::check() {
  if (verbose_) {
    cout << "Checking formula: \n" << to_check_->to_string() << endl;
    print_formulas_id(to_check_);
  }

  if (to_check_->oper() == aalta_formula::True) {
    if (evidence_ != NULL)
      evidence_->push(true);
    return true;
  }
  if (to_check_->oper() == aalta_formula::False)
    return false;
#warning "MR: Here disabled check_with_heuristics"
  RES ret = UNKNOW;  // check_with_heuristics ();
  if (ret != UNKNOW)
    return (ret == SAT ? true : false);

  return dfs_check(to_check_);
}

// check sat by olg heuristics
LTLfChecker::RES LTLfChecker::olg_sat(aalta_formula* f, bool keep_evidence) {
  olg_formula olg(f);
  if (olg.sat()) {
    if (evidence_ != NULL && keep_evidence)
      evidence_->push(olg);
    return SAT;
  }
  return UNSAT;
}

// check unsat by olg heuristics
LTLfChecker::RES LTLfChecker::olg_unsat(aalta_formula* f, bool keep_evidence) {
  olg_formula olg(f);
  if (olg.unsat())
    return SAT;
  return UNSAT;
}

LTLfChecker::RES LTLfChecker::check_with_heuristics() {
  if (to_check_->is_global())  // for global formulas
  {
    aalta_formula* afg = to_check_->ofg();
    if (verbose_)
      cout << "Heuristics for Global formulas:\n";
    return olg_sat(afg, true);
  }

  if (contain_global(to_check_)) {
    if (verbose_)
      cout << "Heuristics for global part unsat:\n";
    if (global_part_unsat(to_check_))
      return UNSAT;
  }
  /*
    if (verbose_)
    cout << "Heuristics for computing off:\n";
    aalta_formula *aaf = to_check_->off();
    if (olg_sat (aaf))
    return SAT;
  */
  if (to_check_->is_wnext_free())  // weak Next free
  {
    if (verbose_)
      cout << "Heuristics for LTL unsatisfiability checking\n";
    if (olg_unsat(to_check_, false) == SAT)
      return UNSAT;
  }
  return UNKNOW;
}

bool LTLfChecker::dfs_check(aalta_formula* f) {
  visited_.push_back(f);
  if (detect_unsat())
    return false;
  if (sat_once(f)) {
    if (verbose_)
      cout << "sat once is true, return from here\n";
    return true;
  } else if (f->is_global()) {
    visited_.pop_back();
    push_formula_to_explored(f);
    if (verbose_)
      cout << "sat once is false and it is global, return from here\n";
    return false;
    ;
  }

  // heuristics: if the global parts of f is unsat, then f is unsat
  /*
    if(contain_global (f))
    {
    if (global_part_unsat (f))
    {
    if (verbose_)
    cout << "Global parts are unsat" << endl;
    visited_.pop_back ();
    push_uc_to_explored ();
    return false;
    }
    }
  */

  // The SAT solver cannot return f as well
  push_formula_to_explored(f);

  while (true) {
    if (detect_unsat())
      return false;
    Transition* t = get_one_transition_from(f);

    if (t != NULL) {
      if (verbose_)
        cout << "getting transition:\n"
             << t->label()->to_string() << " -> " << t->next()->to_string() << endl;
      if (evidence_ != NULL)
        evidence_->push(t->label());
      if (dfs_check(t->next())) {
        delete t;
        return true;
      }
      if (evidence_ != NULL)
        evidence_->pop_back();
    } else {  // cannot get new states, that means f is not used anymore
      if (verbose_)
        cout << "get a null transition\n";
      visited_.pop_back();
      push_uc_to_explored();
      delete t;
      return false;
    }
  }
  visited_.pop_back();
  return false;
}

// check whether there is a conjunct of \@ f is global
bool LTLfChecker::contain_global(aalta_formula* f) {
  if (f->is_global())
    return true;
  else if (f->oper() == aalta_formula::And)
    return contain_global(f->l_af()) || contain_global(f->r_af());
  return false;
}

bool LTLfChecker::global_part_unsat(aalta_formula* f) {
  bool ret = solver_->solve_with_global_assumption(f);
  if (!ret)
    return true;
  return false;
}

Transition* LTLfChecker::get_one_transition_from(aalta_formula* f) {
  bool ret = solver_->solve_by_assumption(f);
  if (ret) {
    Transition* res = solver_->get_transition();
    return res;
  }
  return NULL;
}

void LTLfChecker::push_formula_to_explored(aalta_formula* f) {
  solver_->block_formula(f);
}

void LTLfChecker::push_uc_to_explored() {
  solver_->block_uc();
}

bool LTLfChecker::sat_once(aalta_formula* f) {
  if (solver_->check_tail(f)) {
    if (evidence_ != NULL) {
      Transition* t = solver_->get_transition();
      assert(t != NULL);
      evidence_->push(t->label());
      delete t;
    }
    return true;
  }
  return false;
}

void LTLfChecker::print_evidence() {
  assert(evidence_ != NULL);
  evidence_->print();
}

void LTLfChecker::print_uc() {
  std::vector<int> u = solver_->get_uc();
  for (auto it = u.begin(); it != u.end(); it++) {
    int            id = abs(*it);
    aalta_formula* f  = solver_->get_ass_formula(id);
    if (f != NULL) {
      cout << " ";
      if (*it < 0)
        cout << "!";
      cout << f->to_string() /*<< " (" << id << ")"*/;
      uc_size_++;
    }
  }
}
unsigned int LTLfChecker::get_uc_size() {
  return uc_size_;
}

void LTLfChecker::print_mus() {
  std::vector<int> u = solver_->get_mus({});
  for (auto it = u.begin(); it != u.end(); it++) {
    int            id = abs(*it);
    aalta_formula* f  = solver_->get_ass_formula(id);
    if (f != NULL) {
      cout << " ";
      if (*it < 0)
        cout << "!";
      cout << f->to_string();
      mus_size_++;
    }
  }
}
unsigned int LTLfChecker::get_mus_size() {
  return mus_size_;
}

void LTLfChecker::print_all_mus() {
  std::vector<std::vector<int>> all_mus   = solver_->enumerate_all_mus();
  int                           mus_count = 1;

  for (const auto& mus : all_mus) {
    int len = 0;
    cout << "-- MUS #" << mus_count << ": ";
    for (auto it = mus.begin(); it != mus.end(); ++it) {
      int            id = abs(*it);
      aalta_formula* f  = solver_->get_ass_formula(id);

      if (f != NULL) {
        if (*it < 0)
          cout << "!";
        cout << f->to_string() /*<< " (" << id << ")"*/;
        len++;
        if (std::next(it) != mus.end()) {
          cout << " ";
        }
      }
    }

    cout << "\n-- Mus size: " << len << "\n";
    mus_count++;
  }
}

void LTLfChecker::print_formulas_id(aalta_formula* f) {
  if (f == NULL)
    return;
  cout << f->id() << " : " << f->to_string() << endl;
  print_formulas_id(f->l_af());
  print_formulas_id(f->r_af());
}

// p8.ltl: 9 (P0) 14 (P1)
void LTLfChecker::enumerate_all_mus_v2(std::vector<aalta_formula*>& formulas) {
  external_assumptions_ = get_external_assumptions(to_check_);

  std::vector<int> mus = solver_->get_mus({});

  if (!mus.empty()) {
    std::cout << "-- Initial MUS found: ";
    print_mus(mus);

    bool_solver_ = new AaltaSolver(verbose_);
    initialize_bool_solver(external_assumptions_);

    block_up(mus);

    while (true) {
      if (!bool_solver_->solve_assumption()) {
        break;
      }

      std::vector<int> new_assumptions = get_model_assumptions();

      CARChecker* new_checker = new CARChecker(to_check_, verbose_);
      new_checker->add_assumptions(formulas);

      Minisat::vec<Minisat::Lit> custom_assumptions;
      for (int assumption : new_assumptions) {
        custom_assumptions.push(new_checker->solver_->SAT_lit(assumption));
      }

      if (!new_checker->check()) {
        std::vector<int> new_mus = extract_single_mus(to_check_, new_assumptions);
        if (!new_mus.empty()) {
          std::cout << "-- Additional MUS found: ";
          print_mus(new_mus);
          block_up(new_mus);
        }
      } else {
        block_down(new_assumptions);
      }

      delete new_checker;
      break;
    }
    delete bool_solver_;
  }
}

std::vector<int> LTLfChecker::extract_single_mus(aalta_formula*          gamma,
                                                 const std::vector<int>& new_assumptions) {
  Minisat::vec<Minisat::Lit> custom_assumptions;

  for (int ext_ass : external_assumptions_) {
    custom_assumptions.push(solver_->SAT_lit(ext_ass));
  }

  for (int new_ass : new_assumptions) {
    custom_assumptions.push(solver_->SAT_lit(new_ass));
  }

  return solver_->get_mus(custom_assumptions);
}

void LTLfChecker::print_mus(const std::vector<int>& mus) {
  for (int id : mus) {
    aalta_formula* f = solver_->get_ass_formula(abs(id));
    if (f != NULL) {
      if (id < 0)
        cout << "!";
      cout << f->to_string() << " ";
    }
  }
  cout << endl;
}

bool LTLfChecker::block_up(const std::vector<int>& mus) {
  // Add clause ¬a1 ∨ ¬a2 ∨ ... ∨ ¬an where ai are literals in MUS
  std::vector<int> clause;
  for (int lit : mus) {
    clause.push_back(-lit);  // Negate each literal
  }
  bool_solver_->add_clause(clause);
  return true;
}

bool LTLfChecker::block_down(const std::vector<int>& assumptions) {
  // Add clause for variables not in assumptions
  std::vector<int> clause;
  std::vector<int> all_vars = get_all_assumption_vars();

  for (int var : all_vars) {
    if (std::find(assumptions.begin(), assumptions.end(), var) == assumptions.end()) {
      clause.push_back(var);
    }
  }

  bool_solver_->add_clause(clause);
  return true;
}

std::vector<int> LTLfChecker::get_model_assumptions() {
  std::vector<int> model = bool_solver_->get_model();
  std::vector<int> assumptions;

  // Only include variables assigned true
  for (int i = 0; i < model.size(); i++) {
    if (model[i] > 0) {
      assumptions.push_back(i + 1);
    }
  }

  // Set indifferent variables to true
  std::vector<int> all_vars = get_all_assumption_vars();
  for (int var : all_vars) {
    if (std::find(model.begin(), model.end(), var) == model.end()
        && std::find(model.begin(), model.end(), -var) == model.end()) {
      assumptions.push_back(var);
    }
  }

  return assumptions;
}

void LTLfChecker::initialize_bool_solver(const std::vector<int>& ext_assumptions) {
  // Initialize boolean solver with variables for each assumption
  for (int i = 0; i < ext_assumptions.size(); i++) {
    bool_solver_->newVar();
  }
}

std::vector<int> LTLfChecker::get_external_assumptions(aalta_formula* gamma) {
  std::vector<int>          assumptions;
  aalta_formula::af_prt_set ands = gamma->to_set();

  for (aalta_formula::af_prt_set::iterator it = ands.begin(); it != ands.end(); ++it) {
    assumptions.push_back(solver_->SAT_id(*it));
  }

  return assumptions;
}

std::vector<int> LTLfChecker::get_all_assumption_vars() {
  std::vector<int> vars;
  if (bool_solver_) {
    for (int i = 0; i < bool_solver_->ext_assumption_.size(); i++) {
      vars.push_back(bool_solver_->lit_id(bool_solver_->ext_assumption_[i]));
    }
  }
  return vars;
}
}  // namespace aalta