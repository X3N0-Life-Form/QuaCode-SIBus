/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *    Vincent Barichard <Vincent.Barichard@univ-angers.fr>
 *
 *  Copyright:
 *    Vincent Barichard, 2013
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <iostream>
#include <iomanip>
#include <list>
#include <vector>
#include <sibus/receivers/receiver-compil-certif.hh>

#define OSTREAM std::cerr

ProcessCOMPIL::ProcessCOMPIL(bool _bModuloProp) : Receiver(), bModuloProp(_bModuloProp), failure(false), instance(false) { }

void ProcessCOMPIL::eventVar(const TVarInfo& var) {
  if (var.inBinder) binder.push_back(var);
  else varAux.push_back(var);
}

void ProcessCOMPIL::eventVarArray(unsigned int size, const TVarInfo& var) {
  if (var.inBinder) {
    for (unsigned int i=0; i<size; i++) binder.push_back(var);
  } else {
    for (unsigned int i=0; i<size; i++) varAux.push_back(var);
  }
}

void ProcessCOMPIL::eventCloseModeling() {
}

void ProcessCOMPIL::eventChoice(int idx, TVal val) {
  TVarInfo v = binder[idx];
  v.dom = val;

  if (instance) { // True if last event is an eventInstance
    // Backtrack scenario
    if (!scenario.empty()) {
      while (scenario.back().id != v.id) scenario.pop_back();
      scenario.pop_back();
    }
    instance = false;
  }

  if (failure) { // True if last event is an eventFailure
    // We backup failed scenario
    failedScenario = scenario;
    // Backtrack scenario
    if (!scenario.empty()) {
      while (scenario.back().id != v.id) scenario.pop_back();
      scenario.pop_back();
    }
    failure = false;
  }

  // When branching several times on the same variable, we kept
  // only the last one. It occurs with the INT_VAL_MIN branching heuristic.
  if (!scenario.empty() && (scenario.back().id == v.id)) scenario.pop_back();
  scenario.push_back(v);

  if (!failedScenario.empty()) {
    // When a new node will be opened, we will
    // remove all scenario which are prefix
    if (lsc.empty()) {
      failedScenario.clear();
    } else {
      assert(scenario.size() <= failedScenario.size());
      // Check if "scenario" is prefix of "failedscenario"
      TScenario::const_iterator itSc =     scenario.begin();
      TScenario::const_iterator itEndSc =  scenario.end();
      TScenario::const_iterator itScF =    failedScenario.begin();
      TScenario::const_iterator itEndScF = failedScenario.end();
      bool prefix = true;
      for ( ; prefix && (itSc != itEndSc) ; ++itSc, ++itScF)
        prefix = ((itScF != itEndScF)) && ((*itSc).id == (*itScF).id) && ((*itSc).dom == (*itScF).dom);
      if (!prefix) {
        // Failed scenarios have do be removed NOW !
        std::list<TScenario>::iterator itLsc =    lsc.begin();
        std::list<TScenario>::iterator itEndLsc = lsc.end();
        while ( itLsc != itEndLsc ) {
          // Check if "failedScenario" is prefix of *it
          itEndScF =                         itScF;
          itScF =                            failedScenario.begin();
          TScenario::const_iterator it =     (*itLsc).begin();
          TScenario::const_iterator itEnd =  (*itLsc).end();
          bool prefix = true;
          for ( ; prefix && (itScF != itEndScF) ; ++it, ++itScF) {
            // We drop variables of recorded instance until the variable
            // matches the one of the failed scenario.
            // The size of recorded instance and failed scenario may be different
            // if we use the full instances to build list of scenarios instead of
            // scenario deduced from the choices of the trace.
            while ((it != itEnd) && ((*it).id != (*itScF).id)) ++it;
            prefix = (it != itEnd) && ((*it).dom == (*itScF).dom);
          }
          if (prefix) {
            itLsc = lsc.erase(itLsc);
          } else {
            ++itLsc;
          }
        }
        failedScenario.clear();
      }
    }
  }
}

void ProcessCOMPIL::eventFailure() {
  assert(failedScenario.empty());
  failure = true;
}

void ProcessCOMPIL::eventInstance(const TInstance& _instance) {
  assert(failedScenario.empty());
  instance = true;

  if (bModuloProp) {
    lsc.push_front(scenario);
  } else {
    assert(binder.size() == _instance.size());
    std::list<TVarInfo> newScenario;
    // Create scenario from instance
    std::vector<TVal>::const_iterator itInstance = _instance.begin();
    std::vector<TVarInfo>::const_iterator it     = binder.begin();
    std::vector<TVarInfo>::const_iterator itEnd  = binder.end();
    for ( ; it != itEnd ; ++it, ++itInstance ) {
      newScenario.push_back(*it);
      newScenario.back().dom = *itInstance;
    }
    lsc.push_front(newScenario);
  }
}

void updateGuard(TVarInfo v, const std::list<TVarInfo>& comb, TGuard& g) {
  TGuard::iterator it =    g.begin();
  TGuard::iterator itEnd = g.end();
  for ( ; it != itEnd ; ++it ) {
    if ((*it).v == v) {
      (*it).a.add(comb);
      return;
    }
  }
  // No variable 'v' has been found
  Tree<TVarInfo> a;
  a.add(comb);
  g.insert(g.begin(), TGuardElt(v, a));
}

void ProcessCOMPIL::insertScenario(TScenario sc, std::list<TVGuard>& lg) {
  std::list<TVarInfo> comb;
  comb.push_back(TVarAux()); // Insert a useless node which will play the role of the root in the tree

  while (!sc.empty()) {
    TVarInfo headSc = sc.front();
    // If there is many consecutive same variables in the scenario,
    // we kept the last one, the most precise. Occurs when branching doesn't
    // instantiate a value (for example a INT_VAL_MIN())
    while (headSc.id == sc.front().id) {
      headSc = sc.front();
      sc.pop_front();
    }

    if (headSc.q == FORALL) {
      comb.push_back(headSc);
    } else {
      // We are looking for the next guard involved sc.front()
      std::list<TVGuard>::iterator it    = lg.begin();
      std::list<TVGuard>::iterator itEnd = lg.end();
      for ( ; (it != itEnd) && ((*it).x.id != headSc.id) ; ++it);
      if (it == itEnd)
        lg.push_back(TVGuard(headSc));
      else
        updateGuard(headSc,comb,(*it).g);
    }
  }
}

void ProcessCOMPIL::eventSuccess() {
  std::list<TVGuard> lg;

  // Build base (Algorithm 4 (of the paper): construit_base)
  std::list<TScenario>::const_reverse_iterator it =    lsc.rbegin();
  std::list<TScenario>::const_reverse_iterator itEnd = lsc.rend();
  for ( ; it != itEnd ; ++it ) {
    const TScenario& sc = (*it);
    insertScenario(sc,lg);
    // Print scenario for debug purpose
    TScenario::const_iterator itS =    sc.begin();
    TScenario::const_iterator itSEnd = sc.end();
    for ( ; itS != itSEnd ; ++itS) {
      if ((*itS).dom.type == VAL_NONE)
        OSTREAM << " val(" << (*itS).name << ",NOVALUE)";
      else if ((*itS).dom.type == VAL_BOOL)
        OSTREAM << " val(" << (*itS).name << "," << (*itS).dom.val.b << ")";
      else if ((*itS).dom.type == VAL_INT)
        OSTREAM << " val(" << (*itS).name << "," << (*itS).dom.val.z << ")";
      else
        OSTREAM << " val(" << (*itS).name << ",[" << (*itS).dom.val.bounds[0] << ":" << (*itS).dom.val.bounds[1] << "])";
    }
    OSTREAM << std::endl;
  }

  OSTREAM << std::endl;
  OSTREAM << "CONSTRAINTS" << std::endl;
  // Build the table constraints (Algorithm 8)
  std::list<TVGuard>::const_iterator itLg =    lg.begin();
  std::list<TVGuard>::const_iterator itEndLg = lg.end();
  for ( ; itLg != itEndLg ; ++itLg ) {
    TGuard::const_iterator itG =    (*itLg).g.begin();
    TGuard::const_iterator itEndG = (*itLg).g.end();
    for ( ; itG != itEndG ; ++itG ) {
      if ((*itG).v.dom.type == VAL_NONE) continue;
      OSTREAM << std::setw(3) << (*itLg).x.name << "(";
      if ((*itG).v.dom.type == VAL_NONE)
        OSTREAM << "NOVALUE";
      else if ((*itG).v.dom.type == VAL_BOOL)
        OSTREAM << (*itG).v.dom.val.b;
      else if ((*itG).v.dom.type == VAL_INT)
        OSTREAM << (*itG).v.dom.val.z;
      else
        OSTREAM << "[" << (*itG).v.dom.val.bounds[0] << ":" << (*itG).v.dom.val.bounds[1] << "]";

      if ((*itG).a.nodes() <= 1) {
        OSTREAM << ")" << std::endl;
        continue;
      }
      OSTREAM << ") <- ";
      Tree<TVarInfo>::BranchIterator itA =    (*itG).a.branchBegin();
      Tree<TVarInfo>::BranchIterator itEndA = (*itG).a.branchEnd();
      bool firstBranch = true;
      for ( ; itA != itEndA ; ++itA ) {
        if (!firstBranch) OSTREAM << " || ";
        firstBranch = false;
        OSTREAM << "(";
        std::list<TVarInfo>::const_iterator itN =    (*itA).begin();
        std::list<TVarInfo>::const_iterator itEndN = (*itA).end();
        bool firstValue = true;
        ++itN; // The first node is useless, it is only the root, so we drop it
        for ( ; itN != itEndN ; ++itN ) {
          if (!firstValue) OSTREAM << " && ";
          firstValue = false;
          OSTREAM << (*itN).name << "(";
          if ((*itN).dom.type == VAL_NONE)
            OSTREAM << "NOVALUE";
          else if ((*itN).dom.type == VAL_BOOL)
            OSTREAM << (*itN).dom.val.b;
          else if ((*itN).dom.type == VAL_INT)
            OSTREAM << (*itN).dom.val.z;
          else
            OSTREAM << "[" << (*itN).dom.val.bounds[0] << ":" << (*itN).dom.val.bounds[1] << "]";
          OSTREAM << ")";
        }
        OSTREAM << ")";
      }
      OSTREAM << std::endl;
    }
  }
  OSTREAM << std::endl;
}

void ProcessCOMPIL::eventGlobalFailure() {
}

void ProcessCOMPIL::eventConstraint(TConstraintType , const std::vector<TArg>& ) { }
void ProcessCOMPIL::eventDomainDescription(unsigned int , const std::vector<TVal>& ) { }
void ProcessCOMPIL::eventSwapRequest(unsigned int , unsigned int , unsigned int ) { }
void ProcessCOMPIL::eventSwapDone(unsigned int , unsigned int , unsigned int ) { }
ProcessCOMPIL::~ProcessCOMPIL() { }

