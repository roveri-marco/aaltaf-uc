/*
 * File:   aaltasolver.cpp
 * Author: Jianwen Li
 * Note: An inheritance class from Minisat::Solver for Aalta use
 * Created on August 15, 2017
 */

#include "aaltasolver.h"
#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <unordered_set>
#include <algorithm>
using namespace std;
using namespace Minisat;

namespace aalta
{

  Lit AaltaSolver::SAT_lit (int id)
  {
    assert (id != 0);
    int var = abs(id)-1;
    while (var >= nVars()) newVar();
    return ( (id > 0) ? mkLit(var) : ~mkLit(var) );
  }

  int AaltaSolver::lit_id (Lit l)
  {
    if (sign(l))
      return -(var(l) + 1);
    else
      return var(l) + 1;
  }

  bool AaltaSolver::solve_assumption ()
  {
    Minisat::vec<Minisat::Lit> _ass;
    ext_assumption_.copyTo(_ass);
    for(int i = 0; i < assumption_.size(); i++) {
      _ass.push(assumption_[i]);
    }
    lbool ret = solveLimited(_ass);
    if (verbose_) {
      cout << "solve_with_assumption: assumption_ is" << endl;
      for (int i = 0; i < assumption_.size (); i ++)
	cout << lit_id (assumption_[i]) << ", ";
      cout << endl;
    }
    if (ret == l_True)
      return true;
    else if (ret == l_Undef)
      exit (0);
    return false;
  }

  //return the model from SAT solver when it provides SAT
  std::vector<int> AaltaSolver::get_model ()
  {
    std::vector<int> res;
    res.resize (nVars (), 0);
    for (int i = 0; i < nVars (); i ++)
      {
	if (model[i] == l_True)
	  res[i] = i+1;
	else if (model[i] == l_False)
	  res[i] = -(i+1);
      }
    if (verbose_)
      {
	cout << "original model from SAT solver is" << endl;
	for (int i = 0; i < res.size (); i ++)
	  cout << res[i] << ", ";
	cout << endl;
      }
    return res;
  }

  // return the minimal unsatisfiable subset from SAT solver when it provides SAT
  // 1. assumption_ sempre dentro modulo rimozione di eventuali memberi che sono in ext_assumption_
  // assumption_ \setminus ext_assumption_ sempre dentro
  // play solo con ext_assumption_
  // 2. eventuale ottimizzazione solo con quelle ext_assumption_ che sono in get_mus()
  //
  // X. Portare loop a top level operando solo sulle ext_assumption_ indipendentemente dal core
  std::vector<int> AaltaSolver::get_mus(const Minisat::vec<Minisat::Lit>& custom_ext_assumption) {
    Minisat::vec<Minisat::Lit> _ass;  
    if (custom_ext_assumption.size() == 0) {
        ext_assumption_.copyTo(_ass);
    } else {
        custom_ext_assumption.copyTo(_ass);
    }

    for (int i = 0; i < assumption_.size(); i++) {
        _ass.push(assumption_[i]);
    }

    if (verbose_) {
        std::cout << "Initial assumptions: ";
        for (int i = 0; i < _ass.size(); i++) {
            std::cout << lit_id(_ass[i]) << " ";
        }
        std::cout << "\n";
    }

    if (solve(_ass)) {
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
            std::cout << "--\nTesting assumptions excluding lit: " << lit_id(removed) << "\n";
            std::cout << "Test assumptions: ";
            for (int k = 0; k < _ass.size(); k++) {
                std::cout << lit_id(_ass[k]) << " ";
            }
            std::cout << "\n";
        }

        if (solve(_ass)) {
            mus_set.insert(lit_id(removed));
            _ass.push(removed);
            i++;
            if (verbose_) {
                std::cout << "Literal " << lit_id(removed) << " is part of the MUS\n";
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

  int AaltaSolver::litVectorToHash(const Minisat::vec<Minisat::Lit>& v) {
      std::size_t hash = 0;
      for (int i = 0; i < v.size(); i++) {
          hash = hash ^ (std::hash<int>{}(lit_id(v[i])) + 0x9e3779b9 + (hash << 6) + (hash >> 2));
      }
      return static_cast<int>(hash);
  }

  void AaltaSolver::exploreLiteralCombinations(Minisat::vec<Minisat::Lit>& lits, int index, Minisat::vec<Minisat::Lit>& current, std::set<int>& generated, std::vector<std::vector<int>>& all_mus) {
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


  void AaltaSolver::processPermutations(Minisat::vec<Minisat::Lit>& combination, int start, int end, std::vector<std::vector<int>>& all_mus) {
      if (start == end) {
          Minisat::vec<Minisat::Lit> original_ext_assumptions;
          ext_assumption_.copyTo(original_ext_assumptions);

          ext_assumption_.clear();

          // Only include lits in original_ext_assumptions that are NOT in the current permutation
          for (int j = 0; j < original_ext_assumptions.size(); j++) {
              bool found = false;
              for (int k = 0; k < combination.size(); k++) {
                  if (original_ext_assumptions[j] == combination[k]) {
                      found = true;
                      break;
                  }
              }
              if (!found) {
                  ext_assumption_.push(original_ext_assumptions[j]);
              }
          }

          if(verbose_) {
            cout << "Current combination: ";
            for (int i = 0; i < combination.size(); i++) {
                cout << lit_id(combination[i]) << " ";
            }
            cout << "    Current ext_assumptions_: ";
            for (int i = 0; i < ext_assumption_.size(); i++) {
                cout << lit_id(ext_assumption_[i]) << " ";
            }
            cout << endl;
          }

            auto mus = get_mus();
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

          original_ext_assumptions.copyTo(ext_assumption_);
      } else {
          for (int i = start; i <= end; i++) {
              std::swap(combination[start], combination[i]);
              processPermutations(combination, start + 1, end, all_mus);
              std::swap(combination[start], combination[i]);
          }
      }
  }

  std::vector<std::vector<int>> AaltaSolver::enumerate_all_mus() {
    std::vector<std::vector<int>> all_mus;
    std::set<int> unique_lits;

    std::vector<int> initial_mus = get_mus();
    for (int lit : initial_mus) {
        unique_lits.insert(lit);
    }

    bool found_new;
    do {
        found_new = false;
        Minisat::vec<Minisat::Lit> mus_as_lits;
        for (int lit : unique_lits) {
            mus_as_lits.push(SAT_lit(lit));
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


  //return the UC from SAT solver when it provides UNSAT
  std::vector<int> AaltaSolver::get_uc ()
  {
    std::vector<int> reason;
    if (verbose_)
      cout << "get uc: \n";
    for (int k = 0; k < conflict.size(); k++) {
      Lit l = conflict[k];
      reason.push_back (-lit_id (l));
      if (verbose_)
        cout << -lit_id (l) << ", ";
    }
    if (verbose_)
      cout << endl;
    return reason;
  }

  void AaltaSolver::add_clause (std::vector<int>& v)
  {
    vec<Lit> lits;
    for (std::vector<int>::iterator it = v.begin (); it != v.end (); it ++)
      lits.push (SAT_lit (*it));
    /*
      if (verbose_)
      {
      cout << "Adding clause " << endl << "(";
      for (int i = 0; i < lits.size (); i ++)
      cout << lit_id (lits[i]) << ", ";
      cout << ")" << endl;
      cout << "Before adding, size of clauses is " << clauses.size () << endl;
      }
    */
    addClause (lits);
    /*
      if (verbose_)
      cout << "After adding, size of clauses is " << clauses.size () << endl;
    */
  }

  bool AaltaSolver::contains(const std::vector<std::vector<int>>& all_mus, const std::vector<int>& mus) {
      for (const auto& existing_mus : all_mus) {
          if (is_equal_set(existing_mus, mus)) {
              // cout << "Mus already exists" << endl;
              return true;
          }
      }
      // cout << "Mus does not exist" << endl;
      return false;
  }

  bool AaltaSolver::is_equal_set(const std::vector<int>& set1, const std::vector<int>& set2) {
      if (set1.size() != set2.size()) return false;
      std::unordered_set<int> s1(set1.begin(), set1.end());
      std::unordered_set<int> s2(set2.begin(), set2.end());
      return s1 == s2;
  }


  void AaltaSolver::add_clause (int id)
  {
    std::vector<int> v;
    v.push_back (id);
    add_clause (v);
  }

  void AaltaSolver::add_clause (int id1, int id2)
  {
    std::vector<int> v;
    v.push_back (id1);
    v.push_back (id2);
    add_clause (v);
  }

  void AaltaSolver::add_clause (int id1, int id2, int id3)
  {
    std::vector<int> v;
    v.push_back (id1);
    v.push_back (id2);
    v.push_back (id3);
    add_clause (v);
  }

  void AaltaSolver::add_clause (int id1, int id2, int id3, int id4)
  {
    std::vector<int> v;
    v.push_back (id1);
    v.push_back (id2);
    v.push_back (id3);
    v.push_back (id4);
    add_clause (v);
  }

  void AaltaSolver::print_clauses ()
  {
    cout << "clauses in SAT solver: \n";
    for (int i = 0; i < clauses.size (); i ++)
      {
	Clause& c = ca[clauses[i]];
	for (int j = 0; j < c.size (); j ++)
	  cout << lit_id (c[j]) << ", ";
	cout << endl;
      }
  }
}
