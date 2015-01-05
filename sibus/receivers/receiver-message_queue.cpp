//insert license here

#include<sibus/receivers/receiver-message_queue.hh>

#include <string>

#ifdef SIBUS_THREAD

#define OSTREAM ostream

const char* MessageQueueReceiver::IPC_NAME_TO_ADAPTER = "SIBusShared_toAdapter";
const char* MessageQueueReceiver::IPC_NAME_FROM_ADAPTER = "SIBusShared_fromAdapter";
const int MessageQueueReceiver::MAX_MESSAGES = 40000;
const int MessageQueueReceiver::MESSAGE_SIZE = 200;

MessageQueueReceiver::MessageQueueReceiver() :
  fromAdapterThread(NULL), toAdapterThread(NULL),
  fromAdapterQueue(NULL), toAdapterQueue(NULL),
  bShutdown(false)
{}

MessageQueueReceiver::~MessageQueueReceiver() {

}

void MessageQueueReceiver::setSIBus(SIBus const * _sibus) {
  BoostLock locker(mutex_sibus);
  sibus = _sibus;
  if (_sibus != NULL) {
    fromAdapterQueue = new boost::interprocess::message_queue(
			     boost::interprocess::open_or_create,
			     IPC_NAME_FROM_ADAPTER,
			     MAX_MESSAGES,
			     MESSAGE_SIZE);
    toAdapterQueue = new boost::interprocess::message_queue(
			      boost::interprocess::open_or_create,
			     IPC_NAME_TO_ADAPTER,
			     MAX_MESSAGES,
			     MESSAGE_SIZE);
    fromAdapterThread = new boost::thread(&MessageQueueReceiver::listen, this);
    toAdapterThread = new boost::thread(&MessageQueueReceiver::run, this);
  }
}


void MessageQueueReceiver::manageEvent(const SIBus::TLEvent& e) {
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

  //boost::system::error_code ignored_error;
  //boost::asio::write(*m_socket, boost::asio::buffer(ostream.str()), ignored_error);
  std::string line = ostream.str();
  toAdapterQueue->send(line.data(), line.size(), 0);
  //if (closeSendSocket) m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
}

void MessageQueueReceiver::eventVar(const TVarInfo& var) {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_VAR,var));
}
void MessageQueueReceiver::eventVarArray(unsigned int size, const TVarInfo& var) {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_VARARRAY,size,var));
}

void MessageQueueReceiver::eventCloseModeling() {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_CLOSE_MODELING));
}

void MessageQueueReceiver::eventConstraint(TConstraintType type, const std::vector<TArg>& args) {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_CONSTRAINT,type,args));
  ev_fifo.signal();
}

void MessageQueueReceiver::eventChoice(int idx, TVal val) {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_CHOICE,idx,val));
  ev_fifo.signal();
}

void MessageQueueReceiver::eventFailure() {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_FAILURE));
  ev_fifo.signal();
}

void MessageQueueReceiver::eventGlobalFailure() {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_GLOBAL_FAILURE));
  ev_fifo.signal();
}

void MessageQueueReceiver::eventInstance(const TInstance& instance) {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_INSTANCE,instance));
  ev_fifo.signal();
}

void MessageQueueReceiver::eventSuccess() {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_SUCCESS));
  ev_fifo.signal();
}

void MessageQueueReceiver::eventDomainDescription(unsigned int idVar, const std::vector<TVal>& domain) {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_DOMAIN_DESC,idVar,domain));
  ev_fifo.signal();
}

void MessageQueueReceiver::eventSwapRequest(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_SWAP_ASK,idVar,idVal1,idVal2));
  ev_fifo.signal();
}

void MessageQueueReceiver::eventSwapDone(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) {
  BoostLock locker(mutex);
  if (bShutdown) return;
  m_fifo.push_back(SIBus::TLEvent(EV_SWAP_DONE,idVar,idVal1,idVal2));
  ev_fifo.signal();
}


void MessageQueueReceiver::run(void) {
  // Start working
  for ( ; ; ) {
    mutex.acquire();
    while (!m_fifo.empty()) {
      try {
        manageEvent(m_fifo.front());
        m_fifo.pop_front();
      } catch (std::exception& e) {
        m_fifo.clear();
        bShutdown = true;
        return;
      }
      mutex.release();
      mutex.acquire();
    }
    if (bShutdown) break;
    mutex.release();
    ev_fifo.wait();
  }
  mutex.release();
}

void MessageQueueReceiver::listen(void) {
  for ( ; ; )
  {
    //boost::array<char, 128> buf;
    //boost::system::error_code error;
    std::string line;
    unsigned int receivedSize, priority;
    line.resize(MESSAGE_SIZE);

    //size_t len = m_socket->read_some(boost::asio::buffer(buf), error);
    fromAdapterQueue->receive(&line[0], MESSAGE_SIZE, receivedSize, priority);

    /*if (error == boost::asio::error::eof)
      break; // Connection closed cleanly by peer.
    else if (error)
      throw boost::system::system_error(error); // Some other error.
    */

    if (!sibus) return; // No SIBus, we quit
    //if (len > 0) {
    std::stringstream ssLine(line.data());
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
      mutex_sibus.acquire();
      if (sibus) SIBus::instance().sendSwapAsk(idVar,idVal1,idVal2);
      mutex_sibus.release();
    }
    //}
  }
}

#endif
