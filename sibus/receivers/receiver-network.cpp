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

#include<iostream>
#include<string>
#include<sstream>
#include<sibus/receivers/receiver-network.hh>

#ifdef SIBUS_THREAD

using boost::asio::ip::tcp;
#define OSTREAM ostream

ProcessNetwork::ProcessNetwork() : bShutdown(false) {
  m_socket = NULL;
  m_thread = NULL;
  m_threadListen = NULL;
}

void ProcessNetwork::setSIBus(SIBus const * _sibus) {
  BoostLock locker(mx_sibus);
  sibus = _sibus;

  if (_sibus != NULL) {
    tcp::acceptor acceptor(m_io_service, tcp::endpoint(tcp::v4(), 5555));
    m_socket = new tcp::socket(m_io_service);
    acceptor.accept(*m_socket);
    m_thread = new boost::thread(&ProcessNetwork::run,this);
    m_threadListen = new boost::thread(&ProcessNetwork::listen,this);
  }
}

void ProcessNetwork::manageEvent(const SIBus::TLEvent& e) {
  std::ostringstream ostream;
  bool closeSendSocket = false;
  switch (e.e) {
    case EV_VAR:
    {
      if (e.var.inBinder) {
        binder.push_back(e.var);
        OSTREAM << "VAR_BINDER       =";
      } else {
        varAux.push_back(e.var);
        OSTREAM << "VAR_AUX          =";
      }
      OSTREAM << " var(" << ((e.var.q==EXISTS)?"E":"F") << "," << ((e.var.t==TYPE_BOOL)?"B":"I") << "," << e.var.name;
      switch (e.var.dom.type) {
        case VAL_NONE:
          break;
        case VAL_BOOL:
          OSTREAM << ",bool(" << e.var.dom.val.b << ")";
          break;
        case VAL_INT:
          OSTREAM << ",int(" << e.var.dom.val.z << ")";
          break;
        case VAL_INTERVAL:
          OSTREAM << ",interval(" << e.var.dom.val.bounds[0] << ":" << e.var.dom.val.bounds[1] << ")";
          break;
      }
      OSTREAM << ")" << std::endl;
      break;
    }
    case EV_VARARRAY:
    {
      if (e.var.inBinder) {
        binder.push_back(e.var);
        OSTREAM << "VAR_ARRAY_BINDER =";
      } else {
        varAux.push_back(e.var);
        OSTREAM << "VAR_ARRAY_AUX    =";
      }
      OSTREAM << " size(" << e.size << ")";
      OSTREAM << " var(" << ((e.var.q==EXISTS)?"E":"F") << "," << ((e.var.t==TYPE_BOOL)?"B":"I") << "," << e.var.name;
      switch (e.var.dom.type) {
        case VAL_NONE:
          break;
        case VAL_BOOL:
          OSTREAM << ",bool(" << e.var.dom.val.b << ")";
          break;
        case VAL_INT:
          OSTREAM << ",int(" << e.var.dom.val.z << ")";
          break;
        case VAL_INTERVAL:
          OSTREAM << ",interval(" << e.var.dom.val.bounds[0] << ":" << e.var.dom.val.bounds[1] << ")";
          break;
      }
      OSTREAM << ")" << std::endl;
      break;
    }
    case EV_CLOSE_MODELING:
    {
      OSTREAM << "CLOSE_MODELING" << std::endl;
      break;
    }
    case EV_CONSTRAINT:
    {
      // Names of constraints (useful to convert TConstraintType to string.
      static char s_ComparisonType[][20] = { "NQ", "EQ", "LQ", "LE", "GQ", "GR" };
      static char s_ConstraintTypeName[][20] = { "AND", "OR", "IMP", "XOR", "EQ", "TIMES", "LINEAR", "RE_AND", "RE_OR", "RE_IMP", "RE_XOR", "RE_EQ", "RE_TIMES", "RE_LINEAR", "ELEMENT" };

      OSTREAM << "CONSTRAINT       =";
      OSTREAM << " " << s_ConstraintTypeName[e.cstType];

      std::vector<TArg>::const_iterator it =    e.cstArgs.begin();
      std::vector<TArg>::const_iterator itEnd = e.cstArgs.end();
      for ( ; it != itEnd ; ++it ) {
        switch ((*it).type) {
          case VAL_NONE:
            break;
          case VAL_BOOL:
            OSTREAM << " bool(" << (*it).val.b << ")";
            break;
          case VAL_INT:
            OSTREAM << " int(" << (*it).val.z << ")";
            break;
          case VAL_VAR:
            OSTREAM << " var(" << (*it).name << ")";
            break;
          case VAL_INTERVAL:
            OSTREAM << " interval(" << (*it).val.bounds[0] << ";" << (*it).val.bounds[1] << ")";
            break;
          case VAL_CMP:
            OSTREAM << " " << s_ComparisonType[(*it).cmp];
            break;
        }
      }
      OSTREAM << std::endl;
      break;
    }
    case EV_CHOICE:
    {
      OSTREAM << "CHOICE           =";
      if (e.val.type == VAL_INTERVAL) {
        OSTREAM << " interval(" << binder[e.id].name << "," << e.val.val.bounds[0] << "," << e.val.val.bounds[0] << ")" << std::endl;
      } else {
        if (binder[e.id].t == TYPE_BOOL)
          OSTREAM << " val(" << binder[e.id].name << "," << e.val.val.b << ")" << std::endl;
        else
          OSTREAM << " val(" << binder[e.id].name << "," << e.val.val.z << ")" << std::endl;
      }
      break;
    }
    case EV_FAILURE:
    {
      OSTREAM << "FAIL" << std::endl;
      break;
    }
    case EV_GLOBAL_FAILURE:
    {
      OSTREAM << "GLOBAL_FAILURE" << std::endl;
      closeSendSocket = true;
      break;
    }
    case EV_SUCCESS:
    {
      OSTREAM << "SUCCESS" << std::endl;
      closeSendSocket = true;
      break;
    }
    case EV_INSTANCE:
    {
      OSTREAM << "INSTANCE         =";
      const TInstance& instance = e.instance;
      for (unsigned int i=0; i < instance.size(); i++)
        if (instance[i].type == VAL_NONE)
          OSTREAM << " val(" << binder[i].name << ",NOVALUE)";
        else {
          if (binder[i].t == TYPE_BOOL)
            OSTREAM << " val(" << binder[i].name << "," << instance[i].val.b << ")";
          else
            OSTREAM << " val(" << binder[i].name << "," << instance[i].val.z << ")";
        }
      OSTREAM << std::endl;
      break;
    }
    case EV_DOMAIN_DESC:
    {
      OSTREAM << "DOMAIN           = idVar(" << e.id << ")";
      std::vector<TVal>::const_iterator it =    e.domain.begin();
      std::vector<TVal>::const_iterator itEnd = e.domain.end();
      for ( ; it != itEnd ; ++it ) {
        if (binder[e.id].t == TYPE_BOOL)
          OSTREAM << " " << ((*it).val.b?"true":"false");
        else
          OSTREAM << " " << (*it).val.z;
      }
      OSTREAM << std::endl;
      break;
    }
    case EV_SWAP_ASK:
    {
      OSTREAM << "SWAP_ASK         = idVar(" <<  e.id << ") idVal(" << e.idVal1 << ") idVal(" << e.idVal2 << ")" << std::endl;
      break;
    }
    case EV_SWAP_DONE:
    {
      OSTREAM << "SWAP_DONE        = idVar(" <<  e.id << ") idVal(" << e.idVal1 << ") idVal(" << e.idVal2 << ")" << std::endl;
      break;
    }
    default:
      assert(false);
  }

  boost::system::error_code ignored_error;
  boost::asio::write(*m_socket, boost::asio::buffer(ostream.str()), ignored_error);
  if (closeSendSocket) m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
}

void ProcessNetwork::eventVar(const TVarInfo& var) {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_VAR,var));
}

void ProcessNetwork::eventVarArray(unsigned int size, const TVarInfo& var) {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_VARARRAY,size,var));
}

void ProcessNetwork::eventCloseModeling() {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_CLOSE_MODELING));
}

void ProcessNetwork::eventConstraint(TConstraintType type, const std::vector<TArg>& args) {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_CONSTRAINT,type,args));
  ev_fifo.signal();
}

void ProcessNetwork::eventChoice(int idx, TVal val) {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_CHOICE,idx,val));
  ev_fifo.signal();
}

void ProcessNetwork::eventFailure() {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_FAILURE));
  ev_fifo.signal();
}

void ProcessNetwork::eventGlobalFailure() {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_GLOBAL_FAILURE));
  ev_fifo.signal();
}

void ProcessNetwork::eventInstance(const TInstance& instance) {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_INSTANCE,instance));
  ev_fifo.signal();
}

void ProcessNetwork::eventSuccess() {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_SUCCESS));
  ev_fifo.signal();
}

void ProcessNetwork::eventDomainDescription(unsigned int idVar, const std::vector<TVal>& domain) {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_DOMAIN_DESC,idVar,domain));
  ev_fifo.signal();
}

void ProcessNetwork::eventSwapRequest(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_SWAP_ASK,idVar,idVal1,idVal2));
  ev_fifo.signal();
}

void ProcessNetwork::eventSwapDone(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) {
  BoostLock locker(mx);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_SWAP_DONE,idVar,idVal1,idVal2));
  ev_fifo.signal();
}

void ProcessNetwork::run(void) {
  // Start working
  for ( ; ; ) {
    mx.acquire();
    while (!m_fifo.empty()) {
      try {
        manageEvent(m_fifo.front());
        m_fifo.pop_front();
      } catch (std::exception& e) {
        m_fifo.clear();
        bShutdown = true;
        return;
      }
      mx.release();
      mx.acquire();
    }
    if (bShutdown) break;
    mx.release();
    ev_fifo.wait();
  }
  mx.release();
}

void ProcessNetwork::listen(void) {
  for ( ; ; )
  {
    boost::array<char, 128> buf;
    boost::system::error_code error;

    size_t len = m_socket->read_some(boost::asio::buffer(buf), error);

    if (error == boost::asio::error::eof)
      break; // Connection closed cleanly by peer.
    else if (error)
      throw boost::system::system_error(error); // Some other error.

    if (!sibus) return; // No SIBus, we quit
    if (len > 0) {
      std::stringstream ssLine(buf.data());
      std::string event;
      ssLine >> event;
      if (event == "SWAP_ASK") {
        std::string word;
        unsigned int idVar, idVal1, idVal2, i = 0;
        ssLine >> word; // =
        while (ssLine >> word) {
          int posA = word.find('(',0) + 1;
          int posB = word.find(')',posA);
          std::string argType = word.substr(0,posA-1);
          std::string argVal = word.substr(posA,posB-posA);
          if (argType == "idVar") {
            std::istringstream( argVal ) >> idVar;
          } else if (argType == "idVal") {
            if (i == 0)
              std::istringstream( argVal ) >> idVal1;
            else
              std::istringstream( argVal ) >> idVal2;
            i++;
          }
        }
        mx_sibus.acquire();
        if (sibus) SIBus::instance().sendSwapAsk(idVar,idVal1,idVal2);
        mx_sibus.release();
      }
    }
  }
}

ProcessNetwork::~ProcessNetwork() {
  mx.acquire();
  bShutdown = true;
  ev_fifo.signal();
  mx.release();
  m_thread->join();
  m_threadListen->join();
  delete m_thread;
  m_thread = NULL;
  delete m_threadListen;
  m_threadListen = NULL;
  //m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
  // We close the sending part of the socket when the search ends with
  // a SUCCESS or a GLOBAL_FAILURE
  try {
    m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
  } catch (boost::system::system_error& e) {
    // Occurs if client has ended its side before
  }
  m_socket->close();
  delete m_socket;
  m_socket = NULL;
  assert(m_fifo.empty());
}

#endif
