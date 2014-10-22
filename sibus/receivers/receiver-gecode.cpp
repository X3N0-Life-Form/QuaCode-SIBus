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
#include <sibus/receivers/receiver-gecode.hh>

#define OSTREAM std::cerr

void ReceiverGecode::eventVar(const TVarInfo& ) { }
void ReceiverGecode::eventVarArray(unsigned int, const TVarInfo& ) { }
void ReceiverGecode::eventCloseModeling() { }
void ReceiverGecode::eventConstraint(TConstraintType , const std::vector<TArg>& ) { }
void ReceiverGecode::eventChoice(int , TVal ) { }
void ReceiverGecode::eventFailure() { }
void ReceiverGecode::eventGlobalFailure() { }
void ReceiverGecode::eventInstance(const TInstance& ) { }
void ReceiverGecode::eventSuccess() { }
void ReceiverGecode::eventDomainDescription(unsigned int , const std::vector<TVal>& ) { }
void ReceiverGecode::eventSwapDone(unsigned int , unsigned int , unsigned int) { }

void ReceiverGecode::eventSwapRequest(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) {
#ifdef SIBUS_THREAD
  BoostLock lock(mx);
#endif
  std::cout << "eventSwapRequest: idVar=" << idVar << " idVal1=" << idVal1 << " idVal2=" << idVal2 << std::endl;
  swapRequests[idVar].push_back(Tuple(idVal1,idVal2));
}

void ReceiverGecode::setNbVars(unsigned int n) {
#ifdef SIBUS_THREAD
  BoostLock lock(mx);
#endif
  swapRequests.resize(n);
}

bool ReceiverGecode::emptySwapRequestFor(unsigned int id) {
#ifdef SIBUS_THREAD
  BoostLock lock(mx);
#endif
  return swapRequests[id].empty();
}

Tuple ReceiverGecode::frontSwapRequestFor(unsigned int id) {
#ifdef SIBUS_THREAD
  BoostLock lock(mx);
#endif
  return swapRequests[id].front();
}

void ReceiverGecode::popSwapRequestFor(unsigned int id) {
#ifdef SIBUS_THREAD
  BoostLock lock(mx);
#endif
  swapRequests[id].pop_front();
}

ReceiverGecode::~ReceiverGecode() { }

