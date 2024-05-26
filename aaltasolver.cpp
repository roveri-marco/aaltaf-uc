/*
 * File:   aaltasolver.cpp
 * Author: Jianwen Li
 * Note: An inheritance class from Minisat::Solver for Aalta use
 * Created on August 15, 2017
 */

#include "aaltasolver.h"
#include <iostream>
#include <vector>
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

    if (solveLimited(_ass) != l_False) {
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

        if (solveLimited(_ass) == l_True) {
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

    // Return the minimal unsatisfiable subset
    if (verbose_) {
        std::cout << "Final MUS: ";
        for (const auto& lit : mus_set) {
            std::cout << lit << " ";
        }
        std::cout << "\n";
    }

    return std::vector<int>(mus_set.begin(), mus_set.end());
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

void AaltaSolver::block_mus(const std::vector<int>& mus) {
    vec<Lit> blocking_clause;

    // cout << "Blocking MUS: ";

    for (int lit : mus) {
        // cout << lit << " ";
        Lit negated_lit = SAT_lit(-lit); 
        
        blocking_clause.push(negated_lit);
    }
    // cout << endl;

    addClause(blocking_clause);
}

  std::vector<std::vector<int>> AaltaSolver::enumerate_all_mus() {
    std::vector<std::vector<int>> all_mus;
    auto first_mus = get_mus();
    if (!first_mus.empty()) {
        all_mus.push_back(first_mus);

        if (verbose_) {
            cout << "First mus: ";
            for (int lit : first_mus) {
                cout << lit << " ";
            }
            cout << endl;
        }

        Minisat::vec<Minisat::Lit> original_assumptions;
        for (int i = 0; i < assumption_.size(); i++) {
            original_assumptions.push(assumption_[i]);
        }
        for (int i = 0; i < ext_assumption_.size(); i++) {
            original_assumptions.push(ext_assumption_[i]);
        }

        for (int i = 0; i < first_mus.size(); i++) {
            int lit = first_mus[i];

            Minisat::vec<Minisat::Lit> new_assumptions;
            for (int j = 0; j < original_assumptions.size(); j++) {
                if (lit_id(original_assumptions[j]) != lit) {
                    new_assumptions.push(original_assumptions[j]);
                }
            }

            auto new_mus = get_mus(new_assumptions);
            if (verbose_ && !new_mus.empty()) {
                cout << "New mus: ";
                for (int lit : new_mus) {
                    cout << lit << " ";
                }
                cout << endl;
            }
            if (verbose_ && new_mus.empty()) {
                cout << "No new mus found" << endl;
            }
            if (!new_mus.empty() && !contains(all_mus, new_mus)) {
                all_mus.push_back(new_mus);
            }
        }
    }
    return all_mus;
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
