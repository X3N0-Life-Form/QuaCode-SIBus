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
#include <sibus/receivers/receiver-nodecount.hh>

#define OSTREAM std::cout

void ProcessNodeCount::eventVar(const TVarInfo& var) {
  if (var.inBinder) mNbVar++;
}

void ProcessNodeCount::eventVarArray(unsigned int size, const TVarInfo& var) {
  if (var.inBinder) mNbVar += size;
}

void ProcessNodeCount::eventCloseModeling() {
  mNodes = 0;
  mIdx = -1;
  mCurBranch.resize(mNbVar);
  mBFlag = false;
}

void ProcessNodeCount::eventConstraint(TConstraintType , const std::vector<TArg>& ) { }

void ProcessNodeCount::eventChoice(int idx, TVal val) {
  if (!mBFlag || (mCurBranch[idx] != val)) {
    // The mIdx != idx because when branching several times on the same variable, we kept
    // only the last one. It occurs with the INT_VAL_MIN branching heuristic.
    if (!mBFlag && (mIdx != idx)) {
      mNodes++;
//      OSTREAM << "## Branching on variable idx: " << idx << std::endl;
    }
    mBFlag = false;
    mIdx = idx;
    mCurBranch[mIdx] = val;
  }
}

void ProcessNodeCount::eventFailure() {
  mBFlag = true;
}

void ProcessNodeCount::eventGlobalFailure() {
  OSTREAM << "## Number of opened nodes: " << mNodes << std::endl;
}

void ProcessNodeCount::eventInstance(const TInstance& ) {
  mBFlag = true;
}

void ProcessNodeCount::eventSuccess() {
  OSTREAM << "## Number of opened nodes: " << mNodes << std::endl;
}

void ProcessNodeCount::eventDomainDescription(unsigned int , const std::vector<TVal>& ) { }
void ProcessNodeCount::eventSwapDone(unsigned int , unsigned int , unsigned int) { }
void ProcessNodeCount::eventSwapRequest(unsigned int , unsigned int , unsigned int ) { }

ProcessNodeCount::~ProcessNodeCount() { }

