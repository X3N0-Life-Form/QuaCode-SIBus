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

#include <cassert>

#ifdef SIBUS_THREAD
  /*
   * BoostMutex
   */
  BoostMutex::BoostMutex(void) { }
  void
  BoostMutex::acquire(void) {
    b_m.lock();
  }
  bool
  BoostMutex::tryacquire(void) {
   return  b_m.try_lock();
  }
  void
  BoostMutex::release(void) {
    b_m.unlock();
  }

  /*
   * BoostEvent
   */
  BoostEvent::BoostEvent(void) : p_s(false) { }
  void
  BoostEvent::signal(void) {
    {
      boost::lock_guard<boost::mutex> lock(b_m);
      p_s = true;
    }
    b_c.notify_one();
  }
  void
  BoostEvent::wait(void) {
    boost::unique_lock<boost::mutex> lock(b_m);
    while (!p_s)
      b_c.wait(lock);
    p_s = false;
  }
#endif

void SIBus::setEnableEvent() {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
#endif
  sibusState = S_RUN;
}

void SIBus::setDisableEvent() {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
#endif
  sibusState = S_OFF;
}

void SIBus::addReceiver(Receiver& p) {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
#endif
  if (sibusState > S_RUN) return;
  receivers.push_back(&p);
  p.setSIBus(this);
}

void SIBus::removeReceiver(Receiver& p) {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
#endif
  if (sibusState > S_RUN) return;
  std::vector<Receiver*>::iterator it    = receivers.begin();
  std::vector<Receiver*>::iterator itEnd = receivers.end();
  for ( ; (it != itEnd) && ((*it) != &p) ; ++it);
  if ((*it) == &p) {
    p.setSIBus(NULL);
    receivers.erase(it);
  }
}

bool SIBus::sendVar(const TVarInfo& var) {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState > S_INIT) return false;
  m_fifo.push_back(TLEvent(EV_VAR,var));
  ev_fifo.signal();
#else
  if (sibusState > S_INIT) return false;
  manageEvent(TLEvent(EV_VAR,var));
#endif
  return true;
}

bool SIBus::sendVarArray(unsigned int size, const TVarInfo& var) {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState > S_INIT) return false;
  m_fifo.push_back(TLEvent(EV_VARARRAY,size,var));
  ev_fifo.signal();
#else
  if (sibusState > S_INIT) return false;
  manageEvent(TLEvent(EV_VARARRAY,size,var));
#endif
  return true;
}

void SIBus::sendCloseModeling() {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState > S_INIT) return;
  m_fifo.push_back(TLEvent(EV_CLOSE_MODELING));
  sibusState = S_RUN;
  ev_fifo.signal();
#else
  if (sibusState > S_INIT) return;
  manageEvent(TLEvent(EV_CLOSE_MODELING));
  sibusState = S_RUN;
#endif
}

void SIBus::sendConstraint(TConstraintType type, const std::vector<TArg>& args) {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState > S_INIT) return;
  m_fifo.push_back(TLEvent(EV_CONSTRAINT,type,args));
  ev_fifo.signal();
#else
  if (sibusState > S_INIT) return;
  manageEvent(TLEvent(EV_CONSTRAINT,type,args));
#endif
}

void SIBus::sendChoice(int idx, TVal val) {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState < S_RUN) return;
  m_fifo.push_back(TLEvent(EV_CHOICE,idx,val));
  ev_fifo.signal();
#else
  if (sibusState < S_RUN) return;
  manageEvent(TLEvent(EV_CHOICE,idx,val));
#endif
}

void SIBus::sendFailure() {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState < S_RUN) return;
  m_fifo.push_back(TLEvent(EV_FAILURE));
  ev_fifo.signal();
#else
  if (sibusState < S_RUN) return;
  manageEvent(TLEvent(EV_FAILURE));
#endif
}

void SIBus::sendGlobalFailure() {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState < S_RUN) return;
  m_fifo.push_back(TLEvent(EV_GLOBAL_FAILURE));
  ev_fifo.signal();
#else
  if (sibusState < S_RUN) return;
  manageEvent(TLEvent(EV_GLOBAL_FAILURE));
#endif
}

void SIBus::sendInstance(const TInstance& instance) {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState < S_RUN) return;
  m_fifo.push_back(TLEvent(EV_INSTANCE,instance));
  ev_fifo.signal();
#else
  if (sibusState < S_RUN) return;
  assert(instance.size() == binderSize);
  manageEvent(TLEvent(EV_INSTANCE,instance));
#endif
}

void SIBus::sendSuccess() {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState < S_RUN) return;
  m_fifo.push_back(TLEvent(EV_SUCCESS));
  ev_fifo.signal();
#else
  if (sibusState < S_RUN) return;
  manageEvent(TLEvent(EV_SUCCESS));
#endif
}

void SIBus::sendDomainDescription(unsigned int idVar, const std::vector<TVal>& domain) {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState > S_INIT) return;
  m_fifo.push_back(TLEvent(EV_DOMAIN_DESC,idVar,domain));
  ev_fifo.signal();
#else
  if (sibusState > S_INIT) return;
  manageEvent(TLEvent(EV_DOMAIN_DESC,idVar,domain));
#endif
}

void SIBus::sendSwapAsk(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState < S_RUN) return;
  m_fifo.push_back(TLEvent(EV_SWAP_ASK,idVar,idVal1,idVal2));
  ev_fifo.signal();
#else
  if (sibusState < S_RUN) return;
  manageEvent(TLEvent(EV_SWAP_ASK,idVar,idVal1,idVal2));
#endif
}

void SIBus::sendSwapDone(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) {
#ifdef SIBUS_THREAD
  BoostLock locker(mx);
  if (sibusState < S_RUN) return;
  m_fifo.push_back(TLEvent(EV_SWAP_DONE,idVar,idVal1,idVal2));
  ev_fifo.signal();
#else
  if (sibusState < S_RUN) return;
  manageEvent(TLEvent(EV_SWAP_DONE,idVar,idVal1,idVal2));
#endif
}

void SIBus::manageEvent(const TLEvent& e) {
  std::vector<Receiver*>::iterator it    = receivers.begin();
  std::vector<Receiver*>::iterator itEnd = receivers.end();

  switch (e.e) {
    case EV_VAR:
    {
      TVarInfo v = e.var;
      if (v.inBinder) {
        v.id = binderSize;
        binderSize++;
      }
      for ( ; it != itEnd ; ++it) (*it)->eventVar(v);
      break;
    }
    case EV_VARARRAY:
    {
      TVarInfo v = e.var;
      if (v.inBinder) {
        v.id = binderSize;
        binderSize+=e.size;
      }
      for ( ; it != itEnd ; ++it) (*it)->eventVarArray(e.size,v);
      break;
    }
    case EV_CLOSE_MODELING:
      assert(binderSize > 0);
      for ( ; it != itEnd ; ++it) (*it)->eventCloseModeling();
      break;
    case EV_CONSTRAINT:
      for ( ; it != itEnd ; ++it) (*it)->eventConstraint(e.cstType,e.cstArgs);
      break;
    case EV_CHOICE:
      for ( ; it != itEnd ; ++it) (*it)->eventChoice(e.id,e.val);
      break;
    case EV_FAILURE:
      for ( ; it != itEnd ; ++it) (*it)->eventFailure();
      break;
    case EV_GLOBAL_FAILURE:
      for ( ; it != itEnd ; ++it) (*it)->eventGlobalFailure();
      break;
    case EV_SUCCESS:
      for ( ; it != itEnd ; ++it) (*it)->eventSuccess();
      break;
    case EV_INSTANCE:
      for ( ; it != itEnd ; ++it) (*it)->eventInstance(e.instance);
      break;
    case EV_DOMAIN_DESC:
      for ( ; it != itEnd ; ++it) (*it)->eventDomainDescription(e.id,e.domain);
      break;
    case EV_SWAP_ASK:
      for ( ; it != itEnd ; ++it) (*it)->eventSwapRequest(e.id,e.idVal1,e.idVal2);
      break;
    case EV_SWAP_DONE:
      for ( ; it != itEnd ; ++it) (*it)->eventSwapDone(e.id,e.idVal1,e.idVal2);
      break;
    default:
      assert(false);
  }
}
