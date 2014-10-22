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

#ifndef __RECEIVER_NETWORK_HH__
#define __RECEIVER_NETWORK_HH__

#ifdef SIBUS_THREAD

#include<sibus/sibus.hh>
#include<vector>
#include<boost/asio.hpp>

class SIBUS_VTABLE_EXPORT ProcessNetwork : public Receiver {
  // Threads
  boost::thread * m_thread;
  boost::thread * m_threadListen;

  std::vector<TVarInfo> binder;
  std::vector<TVarInfo> varAux;
  // Mutex for access sibus variable
  BoostMutex mx_sibus;

  // Mutex for access to queue
  BoostMutex mx;
  // Event for push data in queue
  BoostEvent ev_fifo;
  // Flag to know if the thread must shutdown
  bool bShutdown;

  // Service used for server
  boost::asio::io_service m_io_service;
  // Socket used by server
  boost::asio::ip::tcp::socket* m_socket;

  // List of event sent by sibus
  std::list<SIBus::TLEvent> m_fifo;

  // Manage an event and send it to the network
  void manageEvent(const SIBus::TLEvent& e);

  // The function that will be executed in thread that send messages
  void run(void);

  // The function that will be executed in thread that listen for messages
  void listen(void);

  public:
    ProcessNetwork();

    void setSIBus(SIBus const * _sibus);
    void eventVar(const TVarInfo& var);
    void eventVarArray(unsigned int size, const TVarInfo& var);
    void eventCloseModeling();
    void eventConstraint(TConstraintType type, const std::vector<TArg>& args);
    void eventChoice(int idx,TVal val);
    void eventFailure();
    void eventGlobalFailure();
    void eventInstance(const TInstance& instance);
    void eventSuccess();
    void eventDomainDescription(unsigned int idVar, const std::vector<TVal>& domain);
    void eventSwapRequest(unsigned int idVar, unsigned int idVal1, unsigned int idVal2);
    void eventSwapDone(unsigned int idVar, unsigned int idVal1, unsigned int idVal2);
    virtual ~ProcessNetwork();
};

#endif
#endif
