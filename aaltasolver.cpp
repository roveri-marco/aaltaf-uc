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
  std::vector<int> aalta::AaltaSolver::get_mus() {
    if (!solve_assumption()) {
      std::vector<int> mus; 
      
      std::set<int> all_ext_assumptions;
      for (int i = 0; i < ext_assumption_.size(); i++) {
        all_ext_assumptions.insert(lit_id(ext_assumption_[i]));
      }

      std::vector<int> uc = get_uc();
      for (int lit : uc) {
        Minisat::vec<Minisat::Lit> test_assumptions;
          for (int j = 0; j < ext_assumption_.size(); j++) {
            if (lit_id(ext_assumption_[j]) != lit) {
              test_assumptions.push(ext_assumption_[j]);
            }
          }

        for (int i = 0; i < assumption_.size(); i++) {
          if (all_ext_assumptions.find(lit_id(assumption_[i])) == all_ext_assumptions.end()) {
            test_assumptions.push(assumption_[i]);
          }
        }

        if (solveLimited(test_assumptions) == l_True) {
          mus.push_back(lit);
        }
      }
      return mus; 
    } else {
        if(verbose_) std::cout << "Initial formula is satisfiable, no MUS exists.\n";
        return {};
    }
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

    while (true) {
        if (!solve_assumption()) {
            auto mus = get_mus();
            // cout << "Found MUS: ";
            //   for (int lit : mus) {
            //     cout << lit << " ";
            //   }
            // cout << endl;
            if (mus.empty()) {
                break;
            }
            if (!contains(all_mus, mus)) {
                all_mus.push_back(mus);
                block_mus(mus);
                // cout << "Blocking MUS" << endl;
        } else {
            break;
        }
    }
    return all_mus;
  }
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
