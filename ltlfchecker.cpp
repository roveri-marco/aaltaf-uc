/*
 * File:   ltlfchecker.cpp
 * Author: Jianwen Li
 * Note: SAT-based LTLf satisfiability checking
 * Created on June 26, 2017
 */

#include "ltlfchecker.h"

#include <iomanip>
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

std::vector<MUSInfo> LTLfChecker::enumerate_all_mus_v2(std::vector<aalta_formula*>& formulas,
                                                       double first_creation_time,
                                                       double first_check_time) {
  std::vector<MUSInfo> all_mus;

  int total_bool_solver_calls      = 0;
  int total_ltlf_checker_creations = 0;

  int temporal_mus_ltlf_creations = 0;

  auto                          t_mus_start   = std::chrono::high_resolution_clock::now();
  std::vector<std::vector<int>> initial_muses = get_temporal_mus(temporal_mus_ltlf_creations);
  auto                          t_mus_end     = std::chrono::high_resolution_clock::now();
  double                        first_mus_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(t_mus_end - t_mus_start).count() / 1e9;

  if (!initial_muses.empty()) {
    total_ltlf_checker_creations += temporal_mus_ltlf_creations;

    bool_solver_ = new AaltaSolver(verbose_);

    // Create variables for ext_assumption_ literals
    for (int i = 0; i < solver_->ext_assumption_.size(); i++) {
      bool_solver_->newVar();
    }

    // Add all initial MUSes
    for (const auto& mus : initial_muses) {
      all_mus.emplace_back(mus,
                           first_creation_time,
                           first_check_time,
                           first_mus_time,
                           0,
                           temporal_mus_ltlf_creations);
      block_up(mus);
    }

    int current_bool_calls     = 0;
    int current_ltlf_creations = 0;

    while (true) {
      // Get boolean model
      Minisat::vec<Minisat::Lit> bool_assumptions;

      current_bool_calls++;
      total_bool_solver_calls++;

      if (!bool_solver_->solve(bool_assumptions)) {
        break;
      }

      // Map boolean variables to original formulas
      std::vector<aalta_formula*> subset_formulas;
      for (int i = 0; i < solver_->ext_assumption_.size(); i++) {
        if (bool_solver_->model[i] == Minisat::lbool((uint8_t)0)) {  // l_True
          int ext_lit = solver_->lit_id(solver_->ext_assumption_[i]);
          subset_formulas.push_back(solver_->ext_assumption_map_[abs(ext_lit)]);
        }
      }

      auto t_checker_start = std::chrono::high_resolution_clock::now();
      // Create new checker with ONLY selected formulas
      CARChecker* new_checker = new CARChecker(to_check_, verbose_);

      current_ltlf_creations++;
      total_ltlf_checker_creations++;

      new_checker->solver_->ext_assumption_.clear();
      new_checker->add_assumptions(subset_formulas);

      auto   t_check_start = std::chrono::high_resolution_clock::now();
      double checker_creation_time =
          std::chrono::duration_cast<std::chrono::nanoseconds>(t_check_start - t_checker_start)
              .count()
          / 1e9;

      bool   is_unsat    = !new_checker->check();
      auto   t_check_end = std::chrono::high_resolution_clock::now();
      double check_time =
          std::chrono::duration_cast<std::chrono::nanoseconds>(t_check_end - t_check_start).count()
          / 1e9;

      if (is_unsat) {
        // Get MUS from current assumptions
        auto             t_mus_extract_start = std::chrono::high_resolution_clock::now();
        std::vector<int> new_mus             = new_checker->solver_->get_mus({});
        auto             t_mus_extract_end   = std::chrono::high_resolution_clock::now();
        double           mus_extraction_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                         t_mus_extract_end - t_mus_extract_start)
                                         .count()
                                     / 1e9;

        if (!new_mus.empty()) {
          block_up(new_mus);
          all_mus.emplace_back(new_mus,
                               checker_creation_time,
                               check_time,
                               mus_extraction_time,
                               current_bool_calls,
                               current_ltlf_creations);

          current_bool_calls     = 0;
          current_ltlf_creations = 0;
        }
      } else {
        // Block this satisfying combination
        std::vector<int> assumptions;
        for (int i = 0; i < solver_->ext_assumption_.size(); i++) {
          if (bool_solver_->model[i] == Minisat::lbool((uint8_t)0)) {
            assumptions.push_back(solver_->lit_id(solver_->ext_assumption_[i]));
          }
        }
        block_down(assumptions);
      }

      delete new_checker;
    }

    delete bool_solver_;

    cout << "\n====== MUSes Summary ======\n";
    cout << "MUS #\tChecker Creation(s)\tCheck Time(s)\t\tMUS Extraction(s)\tBool Calls\tLTLf "
            "Creations\tMUS Content\n";
    for (size_t i = 0; i < all_mus.size(); i++) {
      cout << left << setw(8) << (i + 1) << setw(24) << fixed << setprecision(6)
           << all_mus[i].checker_creation_time << setw(24) << all_mus[i].checker_check_time
           << setw(24) << all_mus[i].mus_extraction_time << setw(16) << all_mus[i].bool_solver_calls
           << setw(16) << all_mus[i].ltlf_checker_creations;

      for (size_t j = 0; j < all_mus[i].mus.size(); j++) {
        aalta_formula* f = solver_->get_ass_formula(abs(all_mus[i].mus[j]));
        if (f != NULL) {
          if (all_mus[i].mus[j] < 0)
            cout << "!";
          cout << f->to_string();
          if (j < all_mus[i].mus.size() - 1)
            cout << " ";
        }
      }
      cout << "\n";
    }
    cout << endl;

    cout << "\n====== Total Statistics ======\n";
    cout << "Total Boolean Solver Calls: " << total_bool_solver_calls << "\n";
    cout << "Total LTLf Checker Creations: " << total_ltlf_checker_creations << "\n";
    cout << endl;
  }

  return all_mus;
}

bool LTLfChecker::block_up(const std::vector<int>& mus) {
  std::vector<int> clause;
  if (verbose_)
    std::cout << "Blocking MUS: ";

  // Map MUS literals to boolean variables
  for (int i = 0; i < solver_->ext_assumption_.size(); i++) {
    int ext_lit = solver_->lit_id(solver_->ext_assumption_[i]);
    if (std::find(mus.begin(), mus.end(), ext_lit) != mus.end()) {
      clause.push_back(-(i + 1));  // Boolean variable
      if (verbose_)
        std::cout << ext_lit << " ";
    }
  }
  if (verbose_)
    std::cout << std::endl;

  bool_solver_->add_clause(clause);
  return true;
}

bool LTLfChecker::block_down(const std::vector<int>& assumptions) {
  std::vector<int> clause;

  // Add complement literals for assumptions
  for (int i = 0; i < solver_->ext_assumption_.size(); i++) {
    int ext_lit = solver_->lit_id(solver_->ext_assumption_[i]);
    if (std::find(assumptions.begin(), assumptions.end(), ext_lit) == assumptions.end()) {
      clause.push_back(i + 1);  // Boolean variable
    }
  }

  bool_solver_->add_clause(clause);
  return true;
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

std::vector<std::vector<int>> LTLfChecker::get_temporal_mus(int& ltlf_checker_count) {
  ltlf_checker_count = 0;
  std::vector<std::vector<int>> min_muses;
  std::vector<int>              current_mus = solver_->get_mus({});

  if (current_mus.empty()) {
    return min_muses;
  }

  min_muses.push_back(current_mus);
  size_t min_size = current_mus.size();

  // Now minimize and collect all MUSes of minimal size
  for (size_t i = 0; i < current_mus.size();) {
    int current = current_mus[i];

    CARChecker* temp_checker = new CARChecker(to_check_, verbose_);
    ltlf_checker_count++;
    std::vector<aalta_formula*> subset_formulas;

    // Add all formulas except the one we're testing
    for (int id : current_mus) {
      if (id != current) {
        aalta_formula* f = solver_->get_ass_formula(abs(id));
        if (f != NULL) {
          subset_formulas.push_back(f);
        }
      }
    }

    temp_checker->add_assumptions(subset_formulas);
    bool is_sat = temp_checker->check();
    delete temp_checker;

    if (is_sat) {
      // If satisfiable when removed, this formula is needed
      i++;
    } else {
      // Found a smaller MUS
      current_mus.erase(current_mus.begin() + i);

      if (current_mus.size() < min_size) {
        // Found a strictly smaller MUS, clear previous ones
        min_size = current_mus.size();
        min_muses.clear();
        min_muses.push_back(current_mus);
      } else if (current_mus.size() == min_size) {
        // Found another MUS of minimal size
        min_muses.push_back(current_mus);
      }
    }
  }

  return min_muses;
}
}  // namespace aalta