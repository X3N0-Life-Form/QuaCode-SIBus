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

#include <sibus/sibus.hh>

// Initialisation of instance of singleton
SIBus* SIBus::m_pSIBusInstance = NULL;

void Receiver::setSIBus(SIBus const * _sibus) {
  sibus = _sibus;
}

Receiver::~Receiver() { }

SIBus::SIBus() : sibusState(S_INIT), binderSize(0) {
#ifdef SIBUS_THREAD
  m_thread = NULL;
#endif
}

void SIBus::create() {
  assert(m_pSIBusInstance == NULL);
  m_pSIBusInstance = new SIBus();

#ifdef SIBUS_THREAD
  m_pSIBusInstance->m_thread = new boost::thread(&SIBus::run,m_pSIBusInstance);
#endif
}

void SIBus::kill() {
  assert(m_pSIBusInstance != NULL);
  delete m_pSIBusInstance;
  m_pSIBusInstance = NULL;
}

void SIBus::run(void) {
#ifdef SIBUS_THREAD
  // Start working
  for ( ; ; ) {
    mx.acquire();
    while (!m_fifo.empty()) {
      manageEvent(m_fifo.front());
      m_fifo.pop_front();
      mx.release();
      mx.acquire();
    }
    if (sibusState == S_SHUTDOWN) break;
    mx.release();
    ev_fifo.wait();
  }
  mx.release();
#endif
}

SIBus::~SIBus() {
#ifdef SIBUS_THREAD
  mx.acquire();
#endif
  sibusState = S_SHUTDOWN;
#ifdef SIBUS_THREAD
  ev_fifo.signal();
  mx.release();
  m_thread->join();
  delete m_thread;
  m_thread = NULL;
  sibusState = S_OFF;
#endif

  // Remove all receivers
  std::vector<Receiver*>::iterator it    = receivers.begin();
  std::vector<Receiver*>::iterator itEnd = receivers.end();
  for ( ; it != itEnd ; ++it) 
    (*it)->setSIBus(NULL);
  receivers.clear();
}

