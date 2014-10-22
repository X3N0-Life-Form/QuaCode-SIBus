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

#ifndef __RECEIVER_OUT_HH__
#define __RECEIVER_OUT_HH__

#include<sibus/sibus.hh>
#include<vector>

class SIBUS_VTABLE_EXPORT ProcessSTDOUT : public Receiver {
  std::vector<TVarInfo> binder;
  std::vector<TVarInfo> varAux;
  public:
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

    virtual ~ProcessSTDOUT();
};

#endif
