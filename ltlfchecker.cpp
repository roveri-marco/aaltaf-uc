/*
 * File:   ltlfchecker.cpp
 * Author: Jianwen Li
 * Note: SAT-based LTLf satisfiability checking
 * Created on June 26, 2017
 */

#include "ltlfchecker.h"
#include "formula/olg_formula.h"
#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <unordered_set>
#include <algorithm>

using namespace std;

namespace aalta
{
  bool LTLfChecker::check ()
  {
    if (verbose_)
      {
	cout << "Checking formula: \n" << to_check_->to_string() << endl;
	print_formulas_id (to_check_);
      }

    if (to_check_->oper () == aalta_formula::True)
      {
	if (evidence_ != NULL)
	  evidence_->push (true);
	return true;
      }
    if (to_check_->oper () == aalta_formula::False)
      return false;
#warning "MR: Here disabled check_with_heuristics"
    RES ret = UNKNOW; // check_with_heuristics ();
    if (ret != UNKNOW)
      return (ret == SAT ? true : false);

    return dfs_check (to_check_);
  }

  //check sat by olg heuristics
  LTLfChecker::RES LTLfChecker::olg_sat (aalta_formula* f, bool keep_evidence)
  {
    olg_formula olg (f);
    if (olg.sat ())
      {
	if (evidence_ != NULL && keep_evidence)
	  evidence_->push (olg);
	return SAT;
      }
    return UNSAT;
  }

  //check unsat by olg heuristics
  LTLfChecker::RES LTLfChecker::olg_unsat (aalta_formula* f, bool keep_evidence)
  {
    olg_formula olg (f);
    if (olg.unsat ())
      return SAT;
    return UNSAT;
  }


  LTLfChecker::RES LTLfChecker::check_with_heuristics ()
  {
    if(to_check_->is_global()) //for global formulas
      {
	aalta_formula *afg = to_check_->ofg ();
	if (verbose_)
	  cout << "Heuristics for Global formulas:\n";
	return olg_sat (afg, true);
      }


    if (contain_global (to_check_))
      {
	if (verbose_)
	  cout << "Heuristics for global part unsat:\n";
	if (global_part_unsat (to_check_))
	  return UNSAT;
      }
    /*
      if (verbose_)
      cout << "Heuristics for computing off:\n";
      aalta_formula *aaf = to_check_->off();
      if (olg_sat (aaf))
      return SAT;
    */
    if(to_check_->is_wnext_free()) //weak Next free
      {
	if (verbose_)
	  cout << "Heuristics for LTL unsatisfiability checking\n";
	if (olg_unsat (to_check_, false) == SAT)
	  return UNSAT;
      }
    return UNKNOW;
  }

  bool LTLfChecker::dfs_check (aalta_formula* f)
  {

    visited_.push_back (f);
    if (detect_unsat ())
      return false;
    if (sat_once (f)) {
      if (verbose_)
	cout << "sat once is true, return from here\n";
      return true;
    }
    else if (f->is_global ()) {
      visited_.pop_back ();
      push_formula_to_explored (f);
      if (verbose_)
	cout << "sat once is false and it is global, return from here\n";
      return false;;
    }

    //heuristics: if the global parts of f is unsat, then f is unsat
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


    //The SAT solver cannot return f as well
    push_formula_to_explored (f);

    while (true) {
      if (detect_unsat ())
	return false;
      Transition* t = get_one_transition_from (f);

      if (t != NULL) {
	if (verbose_)
	  cout << "getting transition:\n" << t->label ()->to_string() << " -> " << t->next ()->to_string () << endl;
	if (evidence_ != NULL)
	  evidence_->push (t->label ());
	if (dfs_check (t->next ())) {
	  delete t;
	  return true;
	}
	if (evidence_ != NULL)
	  evidence_ ->pop_back ();
      }
      else { //cannot get new states, that means f is not used anymore
	if (verbose_)
	  cout << "get a null transition\n";
	visited_.pop_back ();
	push_uc_to_explored ();
	delete t;
	return false;
      }
    }
    visited_.pop_back ();
    return false;
  }

  //check whether there is a conjunct of \@ f is global
  bool LTLfChecker::contain_global (aalta_formula *f)
  {
    if (f->is_global ())
      return true;
    else if (f->oper () == aalta_formula::And)
      return contain_global (f->l_af ()) || contain_global (f->r_af ());
    return false;
  }

  bool LTLfChecker::global_part_unsat (aalta_formula *f)
  {
    bool ret = solver_->solve_with_global_assumption (f);
    if (!ret)
      return true;
    return false;
  }

  Transition* LTLfChecker::get_one_transition_from (aalta_formula* f)
  {
    bool ret = solver_->solve_by_assumption (f);
    if (ret)
      {
	Transition* res = solver_->get_transition ();
	return res;
      }
    return NULL;
  }

  void LTLfChecker::push_formula_to_explored (aalta_formula* f)
  {
    solver_->block_formula (f);
  }

  void LTLfChecker::push_uc_to_explored ()
  {
    solver_->block_uc ();
  }

  bool LTLfChecker::sat_once (aalta_formula *f)
  {
    if (solver_->check_tail (f))
      {
	if (evidence_ != NULL)
	  {
	    Transition *t = solver_->get_transition ();
	    assert (t != NULL);
	    evidence_->push (t->label ());
	    delete t;
	  }
	return true;
      }
    return false;
  }


  void LTLfChecker::print_evidence ()
  {
    assert (evidence_ != NULL);
    evidence_->print ();
  }

  void LTLfChecker::print_uc() {
    std::vector<int> u = solver_->get_uc();
    for(auto it = u.begin(); it != u.end(); it++) {
      int id = abs(*it);
      aalta_formula * f = solver_->get_ass_formula(id);
      if (f != NULL) {
	cout << " ";
	if (*it < 0) cout << "!";
	cout << f->to_string() /*<< " (" << id << ")"*/;
	uc_size_++;
      }
    }
  }
  unsigned int LTLfChecker::get_uc_size() {
    return uc_size_;
  }


  unsigned int LTLfChecker::get_mus_size() {
    return mus_size_;
  }

void LTLfChecker::print_all_mus() {
    std::vector<std::vector<int>> all_mus = enumerate_all_mus();
    int mus_count = 1;
    
    for (const auto& mus : all_mus) {
        int len = 0;
        cout << "-- MUS #" << mus_count << ": ";
        for (auto it = mus.begin(); it != mus.end(); ++it) {
            int id = abs(*it);
            aalta_formula* f = solver_->get_ass_formula(id);
            
            if (f != NULL) {
                if (*it < 0) cout << "!";
                cout << f->to_string();
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

  void LTLfChecker::print_formulas_id (aalta_formula* f)
  {
    if (f == NULL)
      return;
    cout << f->id () << " : " << f->to_string () << endl;
    print_formulas_id (f->l_af ());
    print_formulas_id (f->r_af ());
  }

  // In ltlfchecker.cpp:
std::vector<int> LTLfChecker::get_mus(const Minisat::vec<Minisat::Lit>& custom_assumption) {
    Minisat::vec<Minisat::Lit> _ass;
    
    if (custom_assumption.size() == 0) {
        solver_->get_ext_assumption().copyTo(_ass);
    } else {
        custom_assumption.copyTo(_ass);
    }

    for (int i = 0; i < solver_->get_assumption().size(); i++) {
        _ass.push(solver_->get_assumption()[i]);
    }

    if (verbose_) {
        std::cout << "Initial assumptions: ";
        for (int i = 0; i < _ass.size(); i++) {
            std::cout << solver_->lit_id(_ass[i]) << " ";
        }
        std::cout << "\n";
    }

    if (solver_->solve(_ass)) {
        if (verbose_) {
            std::cout << "Formula is satisfiable with the given assumptions.\n";
        }
        return std::vector<int>();
    }

    std::set<int> mus_set;

    for (int i = 0; i < _ass.size(); ) {
        Minisat::Lit removed = _ass[i];
        _ass[i] = _ass.last();
        _ass.pop();

        if (verbose_) {
            std::cout << "--\nTesting assumptions excluding lit: " 
                     << solver_->lit_id(removed) << "\n";
            std::cout << "Test assumptions: ";
            for (int k = 0; k < _ass.size(); k++) {
                std::cout << solver_->lit_id(_ass[k]) << " ";
            }
            std::cout << "\n";
        }

        if (solver_->solve(_ass)) {
            mus_set.insert(solver_->lit_id(removed));
            _ass.push(removed);
            i++;
            if (verbose_) {
                std::cout << "Literal " << solver_->lit_id(removed) 
                         << " is part of the MUS\n";
            }
        } else {
            if (verbose_) {
                std::cout << "Test assumptions are unsatisfiable\n";
            }
        }
    }

    if (verbose_) {
        std::cout << "Final MUS: ";
        for (const auto& lit : mus_set) {
            std::cout << lit << " ";
        }
        std::cout << "\n";
    }

    return std::vector<int>(mus_set.begin(), mus_set.end());
}

// We also need to modify the print_mus() function to use our new get_mus():
void LTLfChecker::print_mus() {
    std::vector<int> u = get_mus({});
    for(auto it = u.begin(); it != u.end(); it++) {
        int id = abs(*it);
        aalta_formula * f = solver_->get_ass_formula(id);
        if (f != NULL) {
            cout << " ";
            if (*it < 0) cout << "!";
            cout << f->to_string();
            mus_size_++;
        }
    }
}

int LTLfChecker::litVectorToHash(const Minisat::vec<Minisat::Lit>& v) {
    std::size_t hash = 0;
    int GOLDEN_RATIO = 0x9e3779b9;
    for (int i = 0; i < v.size(); i++) {
        hash = hash ^ (std::hash<int>{}(solver_->lit_id(v[i])) + GOLDEN_RATIO + 
                      (hash << 6) + (hash >> 2));
    }
    return static_cast<int>(hash);
}

void LTLfChecker::exploreLiteralCombinations(
    Minisat::vec<Minisat::Lit>& lits, int index,
    Minisat::vec<Minisat::Lit>& current,
    std::set<int>& generated,
    std::vector<std::vector<int>>& all_mus) {
    
    if (index == lits.size()) {
        if (current.size() > 0) {
            int hashValue = litVectorToHash(current);
            if (generated.find(hashValue) == generated.end()) {
                processPermutations(current, 0, current.size() - 1, all_mus);
                generated.insert(hashValue);
            }
        }
    } else {
        current.push(lits[index]);
        exploreLiteralCombinations(lits, index + 1, current, generated, all_mus);

        current.pop();
        exploreLiteralCombinations(lits, index + 1, current, generated, all_mus);
    }
}

void LTLfChecker::processPermutations(
    Minisat::vec<Minisat::Lit>& combination,
    int start, int end,
    std::vector<std::vector<int>>& all_mus) {
    
    if (true) {
        Minisat::vec<Minisat::Lit> original_ext_assumptions;
        solver_->get_ext_assumption().copyTo(original_ext_assumptions);

        solver_->clear_ext_assumption();

        // Only include lits in original_ext_assumptions that are NOT in the current combination
        for (int j = 0; j < original_ext_assumptions.size(); j++) {
            bool found = false;
            for (int k = 0; k < combination.size(); k++) {
                if (original_ext_assumptions[j] == combination[k]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                solver_->add_to_ext_assumption(original_ext_assumptions[j]);
            }
        }

        if(verbose_) {
            cout << "Current combination: ";
            for (int i = 0; i < combination.size(); i++) {
                cout << solver_->lit_id(combination[i]) << " ";
            }
            cout << "    Current ext_assumptions_: ";
            for (int i = 0; i < solver_->get_ext_assumption().size(); i++) {
                cout << solver_->lit_id(solver_->get_ext_assumption()[i]) << " ";
            }
            cout << endl;
        }

        auto mus = get_mus({});
        if (!empty(mus) && !contains(all_mus, mus)) {
            all_mus.push_back(mus);
            if(verbose_) {
                cout << "New MUS found:";
                for (auto lit : mus) {
                    cout << " " << lit;
                }
                cout << endl;
            }
        }

        original_ext_assumptions.copyTo(solver_->get_ext_assumption());
    }
}

std::vector<std::vector<int>> LTLfChecker::enumerate_all_mus() {
    std::vector<std::vector<int>> all_mus;
    std::set<int> unique_lits;

    std::vector<int> initial_mus = get_mus({});
    for (int lit : initial_mus) {
        unique_lits.insert(lit);
    }

    bool found_new;
    do {
        found_new = false;
        Minisat::vec<Minisat::Lit> mus_as_lits;
        for (int lit : unique_lits) {
            mus_as_lits.push(solver_->SAT_lit(lit));
        }

        std::set<int> generated;
        Minisat::vec<Minisat::Lit> current;
        exploreLiteralCombinations(mus_as_lits, 0, current, generated, all_mus);

        std::set<int> new_lits;
        for (const auto& mus : all_mus) {
            for (int lit : mus) {
                if (unique_lits.insert(lit).second) {
                    new_lits.insert(lit);
                }
            }
        }

        if (!new_lits.empty()) {
            found_new = true;
        }
    } while (found_new);
    return all_mus;
}

bool LTLfChecker::contains(const std::vector<std::vector<int>>& all_mus, 
                          const std::vector<int>& mus) {
    for (const auto& existing_mus : all_mus) {
        if (is_equal_set(existing_mus, mus)) {
            return true;
        }
    }
    return false;
}

bool LTLfChecker::is_equal_set(const std::vector<int>& set1,
                              const std::vector<int>& set2) {
    std::unordered_set<int> valid_lits;
    
    for (int i = 0; i < solver_->get_ext_assumption().size(); i++) {
        valid_lits.insert(solver_->lit_id(solver_->get_ext_assumption()[i]));
    }
    
    std::unordered_set<int> valid_set1, valid_set2;
    
    for (int id : set1) {
        if (valid_lits.find(id) != valid_lits.end()) {
            valid_set1.insert(id);
        }
    }
    
    for (int id : set2) {
        if (valid_lits.find(id) != valid_lits.end()) {
            valid_set2.insert(id);
        }
    }
    
    return valid_set1 == valid_set2;
}
}
