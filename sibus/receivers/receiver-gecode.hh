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

#ifndef __RECEIVER_GECODE_HH__
#define __RECEIVER_GECODE_HH__

#include <sibus/sibus.hh>
#include<list>
#include<vector>

struct Tuple {
  unsigned int v1;
  unsigned int v2;
  Tuple(unsigned int _v1, unsigned int _v2) : v1(_v1), v2(_v2) {}
};

class SIBUS_VTABLE_EXPORT ReceiverGecode : public Receiver {
  // The vector of swap requests
  std::vector< std::list<Tuple> > swapRequests;
#ifdef SIBUS_THREAD
  // Mutex for access to vector of swap requests
  BoostMutex mx;
#endif
  public:
    // Set the size of the swapRequests vector
    void setNbVars(unsigned int n);

    // Answer true is no swap request has been stored for variable id \a id, false otherwise
    bool emptySwapRequestFor(unsigned int id);
    // Returns the first swap request for variable id \a id
    Tuple frontSwapRequestFor(unsigned int id);
    // Remove the first swap request for variable id \a id
    void popSwapRequestFor(unsigned int id);

    void eventVar(const TVarInfo& var);
    void eventVarArray(unsigned int size, const TVarInfo& var);
    void eventCloseModeling();
    void eventConstraint(TConstraintType type, const std::vector<TArg>& args);
    void eventChoice(int idx, TVal val);
    void eventFailure();
    void eventGlobalFailure();
    void eventInstance(const TInstance& instance);
    void eventSuccess();
    void eventDomainDescription(unsigned int idVar, const std::vector<TVal>& domain);
    void eventSwapRequest(unsigned int idVar, unsigned int idVal1, unsigned int idVal2);
    void eventSwapDone(unsigned int idVar, unsigned int idVal1, unsigned int idVal2);

    virtual ~ReceiverGecode();
};

#endif
