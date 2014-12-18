//TODO: put license in there

#ifndef __RECEIVER_MESSAGE_QUEUE_HH__
#define __RECEIVER_MESSAGE_QUEUE_HH__

#ifdef SIBUS_THREAD

#include<sibus/sibus.hh>
#include<vector>
#include<boost/thread.hpp>
#include<boost/interprocess/ipc/message_queue.hpp>

class SIBUS_VTABLE_EXPORT MessageQueueReceiver : public receiver {

  // Threads
  boost::thread* fromAdapterThread;
  boost::thread* toAdapterThread;
  
  boost::interprocess::message_queue* fromAdapterQueue;
  boost::interprocess::message_queue* toAdapterQueue;

  std::vector<TVarInfo> binder;
  std::vector<TVarInfo> varAux;
  // Mutex for access sibus variable
  BoostMutex mutex_sibus;
  // Mutex for access to queue
  BoostMutex mutex;
  // Event for push data in queue
  BoostEvent ev_fifo;
  // Flag to know if the thread must shutdown
  bool bShutdown;

  // List of event sent by sibus
  std::list<SIBus::TLEvent> m_fifo;

  // Manage an event and send it to the network
  void manageEvent(const SIBus::TLEvent& e);

  // The function that will be executed in thread that send messages
  void run(void);

  // The function that will be executed in thread that listen for messages
  void listen(void);

 public:
  // Constants - identical to SIBusAdapter's
  static const char* IPC_NAME_TO_ADAPTER;
  static const char* IPC_NAME_FROM_ADAPTER;
  static const int MAX_MESSAGES;
  static const int MESSAGE_SIZE;

  // Methods
  MessageQueueReceiver();
  virtual ~MessageQueueReceiver();

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
}

#endif
#endif
