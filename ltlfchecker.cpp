/*
 * File:   ltlfchecker.cpp
 * Author: Jianwen Li
 * Note: SAT-based LTLf satisfiability checking
 * Created on June 26, 2017
 */

#include "ltlfchecker.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <set>
#include <tuple>
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

  auto elapsed_since_start = [&t_mus_start]() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::high_resolution_clock::now() - t_mus_start)
               .count()
           / 1e9;
  };

  if (!initial_muses.empty()) {
    total_ltlf_checker_creations += temporal_mus_ltlf_creations;

    // The lines below are candidates reported as they are discovered: each one
    // is unsatisfiable for sure, but minimality is only established at the end,
    // so the summary may drop some of them. Without the closing
    // "enumeration COMPLETE" line these are partial results.
    cout << "-- incremental reporting below, fields: index, elapsed s, size, conjuncts\n";

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
      report_mus(all_mus.size(), mus, elapsed_since_start());
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
          strip_non_ext_ids(new_mus);
          block_up(new_mus);
          if (!new_mus.empty()) {
            all_mus.emplace_back(new_mus,
                                 checker_creation_time,
                                 check_time,
                                 mus_extraction_time,
                                 current_bool_calls,
                                 current_ltlf_creations);
            report_mus(all_mus.size(), new_mus, elapsed_since_start());

            current_bool_calls     = 0;
            current_ltlf_creations = 0;
          }
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

    // A candidate can be a strict superset of a later-found MUS when the
    // boolean shrinking stops early (the clause database of the subset
    // checker under-approximates unsatisfiability): keep only the
    // inclusion-minimal candidates. This does not lose any MUS, since a
    // seed equal to a real MUS is never blocked by block_up/block_down.
    size_t               candidates_reported = all_mus.size();
    std::vector<MUSInfo> minimal_mus;
    for (size_t i = 0; i < all_mus.size(); i++) {
      bool has_strict_subset = false;
      for (size_t j = 0; j < all_mus.size(); j++) {
        if (j != i && all_mus[j].mus.size() < all_mus[i].mus.size()
            && std::includes(all_mus[i].mus.begin(),
                             all_mus[i].mus.end(),
                             all_mus[j].mus.begin(),
                             all_mus[j].mus.end())) {
          has_strict_subset = true;
          break;
        }
      }
      if (!has_strict_subset)
        minimal_mus.push_back(all_mus[i]);
    }
    all_mus.swap(minimal_mus);

    cout << "\n====== MUSes Summary ======\n";
    cout << "MUS #\tChecker Creation(s)\tCheck Time(s)\t\tMUS Extraction(s)\tBool Calls\tLTLf "
            "Creations\tMUS Content\n";
    for (size_t i = 0; i < all_mus.size(); i++) {
      cout << left << setw(8) << (i + 1) << setw(24) << fixed << setprecision(6)
           << all_mus[i].checker_creation_time << setw(24) << all_mus[i].checker_check_time
           << setw(24) << all_mus[i].mus_extraction_time << setw(16) << all_mus[i].bool_solver_calls
           << setw(16) << all_mus[i].ltlf_checker_creations;

      cout << mus_to_string(all_mus[i].mus) << "\n";
    }
    cout << endl;

    cout << "\n====== Total Statistics ======\n";
    cout << "Total Boolean Solver Calls: " << total_bool_solver_calls << "\n";
    cout << "Total LTLf Checker Creations: " << total_ltlf_checker_creations << "\n";

    if (!all_mus.empty()) {
      std::vector<int> mus_sizes;
      for (const auto& mus_info : all_mus) {
        mus_sizes.push_back(mus_info.mus.size());
      }
      
      int min_size = *std::min_element(mus_sizes.begin(), mus_sizes.end());
      int max_size = *std::max_element(mus_sizes.begin(), mus_sizes.end());
      double mean_size = 0.0;
      for (int size : mus_sizes) mean_size += size;
      mean_size /= mus_sizes.size();
      
      double variance_size = 0.0;
      for (int size : mus_sizes) {
        variance_size += (size - mean_size) * (size - mean_size);
      }
      variance_size /= mus_sizes.size();

      cout << "\n====== MUS Size Statistics ======\n";
      cout << "Count: " << all_mus.size() << "\n";
      cout << "Min Size: " << min_size << "\n";
      cout << "Max Size: " << max_size << "\n";
      cout << "Mean Size: " << fixed << setprecision(2) << mean_size << "\n";
      cout << "Variance: " << fixed << setprecision(4) << variance_size << "\n";

      // Calculate timing statistics
      std::vector<double> creation_times, check_times, extraction_times;
      for (const auto& mus_info : all_mus) {
        creation_times.push_back(mus_info.checker_creation_time);
        check_times.push_back(mus_info.checker_check_time);
        extraction_times.push_back(mus_info.mus_extraction_time);
      }

      auto calc_stats = [](const std::vector<double>& times) {
        double min_t = *std::min_element(times.begin(), times.end());
        double max_t = *std::max_element(times.begin(), times.end());
        double mean_t = 0.0;
        for (double t : times) mean_t += t;
        mean_t /= times.size();
        double variance_t = 0.0;
        for (double t : times) variance_t += (t - mean_t) * (t - mean_t);
        variance_t /= times.size();
        return std::make_tuple(min_t, max_t, mean_t, variance_t);
      };

      auto create_stats = calc_stats(creation_times);
      auto check_stats = calc_stats(check_times);
      auto extract_stats = calc_stats(extraction_times);
      
      double min_create = std::get<0>(create_stats), max_create = std::get<1>(create_stats);
      double mean_create = std::get<2>(create_stats), var_create = std::get<3>(create_stats);
      double min_check = std::get<0>(check_stats), max_check = std::get<1>(check_stats);
      double mean_check = std::get<2>(check_stats), var_check = std::get<3>(check_stats);
      double min_extract = std::get<0>(extract_stats), max_extract = std::get<1>(extract_stats);
      double mean_extract = std::get<2>(extract_stats), var_extract = std::get<3>(extract_stats);

      cout << "\n====== Timing Statistics ======\n";
      cout << "Checker Creation Times (s):\n";
      cout << "  Min: " << fixed << setprecision(6) << min_create << "  Max: " << max_create 
           << "  Mean: " << mean_create << "  Variance: " << scientific << setprecision(3) << var_create << "\n";
      cout << "Check Times (s):\n";
      cout << "  Min: " << fixed << setprecision(6) << min_check << "  Max: " << max_check 
           << "  Mean: " << mean_check << "  Variance: " << scientific << setprecision(3) << var_check << "\n";
      cout << "MUS Extraction Times (s):\n";
      cout << "  Min: " << fixed << setprecision(6) << min_extract << "  Max: " << max_extract 
           << "  Mean: " << mean_extract << "  Variance: " << scientific << setprecision(3) << var_extract << "\n";
    }
    cout << endl;

    // Closing marker: if this line is missing the run was cut off by a timeout
    // or by memory exhaustion, and only the MUS-CANDIDATE lines above are left.
    cout << "-- enumeration COMPLETE: " << all_mus.size() << " minimal MUS out of "
         << candidates_reported << " candidates" << endl;
  }

  return all_mus;
}

// Drop ids that do not correspond to external assumption selectors: the
// boolean MUS extraction can also return internal solver assumptions
// (frames, obligations), which are not part of the user-level conjunct set.
void LTLfChecker::strip_non_ext_ids(std::vector<int>& ids) {
  std::vector<int> ext_only;
  for (int id : ids) {
    if (solver_->get_ass_formula(abs(id)) != NULL)
      ext_only.push_back(id);
  }
  ids.swap(ext_only);
}

// Render the user-level conjunct names of a MUS, space separated.
std::string LTLfChecker::mus_to_string(const std::vector<int>& mus) {
  std::string out;
  for (size_t i = 0; i < mus.size(); i++) {
    aalta_formula* f = solver_->get_ass_formula(abs(mus[i]));
    if (f == NULL)
      continue;
    if (!out.empty())
      out += " ";
    if (mus[i] < 0)
      out += "!";
    out += f->to_string();
  }
  return out;
}

void LTLfChecker::report_mus(size_t index, const std::vector<int>& mus, double elapsed) {
  cout << "MUS-CANDIDATE\t" << index << "\t" << fixed << setprecision(6) << elapsed << "\t"
       << mus.size() << "\t" << mus_to_string(mus) << endl;
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
  strip_non_ext_ids(current_mus);

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