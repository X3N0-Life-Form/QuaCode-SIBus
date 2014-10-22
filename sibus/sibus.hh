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

#ifndef __SIBUS_HH__
#define __SIBUS_HH__

/*
 * Configure linking
 *
 */
#if !defined(GECODE_STATIC_LIBS) && \
    (defined(__CYGWIN__) || defined(__MINGW32__) || defined(_MSC_VER))

#ifdef BUILD_SIBUS
#define SIBUS_EXPORT __declspec( dllexport )
#else
#define SIBUS_EXPORT __declspec( dllimport )
#endif

#else

#define SIBUS_VTABLE_EXPORT __attribute__ ((visibility("default")))

#ifdef SIBUS_GCC_HAS_CLASS_VISIBILITY
#define SIBUS_EXPORT __attribute__ ((visibility("default")))
#else
#define SIBUS_EXPORT
#endif

#endif

#include <cassert>
#include <vector>
#include <string>
#include <list>

#ifdef SIBUS_THREAD
  #include <boost/thread.hpp>

  /**
   * \brief A frontend for boost mutex
   */
  class BoostMutex {
  private:
    /// The boost mutex
    boost::mutex b_m;
  public:
    /// Initialize mutex
    inline BoostMutex(void);
    /// Acquire the mutex and possibly block
    inline void acquire(void);
    /// Try to acquire the mutex, return true if succesful
    inline bool tryacquire(void);
    /// Release the mutex
    inline void release(void);
  private:
    /// A mutex cannot be copied
    BoostMutex(const BoostMutex&) {}
    /// A mutex cannot be assigned
    void operator=(const BoostMutex&) {}
  };

  /**
   * \brief A lock as a scoped frontend for a mutex
   */
  class BoostLock {
  private:
    /// The mutex used for the lock
    BoostMutex& m;
  public:
    /// Enter lock
    inline BoostLock(BoostMutex& m0) : m(m0) { m.acquire(); }
    /// Leave lock
    inline ~BoostLock(void) { m.release(); }
  private:
    /// A lock cannot be copied
    BoostLock(const BoostLock& l) : m(l.m) {}
    /// A lock cannot be assigned
    void operator=(const BoostLock&) {}
  };

  /**
   * \brief A frontend for boost condition variable / mutex
   */
  class BoostEvent {
  private:
    /// The boost mutex
    boost::mutex b_m;
    /// The boost condition variable
    boost::condition_variable b_c;
    /// Whether the event is signalled
    bool p_s;
  public:
    /// Initialize event
    inline BoostEvent(void);
    /// Signal the event
    inline void signal(void);
    /// Wait until the event becomes signalled
    inline void wait(void);
  private:
    /// An event cannot be copied
    BoostEvent(const BoostEvent&) {}
    /// An event cannot be assigned
    void operator=(const BoostEvent&) {}
  };

#endif

  #define EV_VAR              0
  #define EV_VARARRAY         1
  #define EV_CLOSE_MODELING   2
  #define EV_CONSTRAINT       3
  #define EV_CHOICE           4
  #define EV_FAILURE          5
  #define EV_GLOBAL_FAILURE   6
  #define EV_SUCCESS          7
  #define EV_INSTANCE         8
  #define EV_DOMAIN_DESC      9
  #define EV_SWAP_ASK        10
  #define EV_SWAP_DONE       11
  // Event type which can take previouses values
  typedef unsigned int TEvent;

  #define EXISTS 0
  #define FORALL 1
  // Type of quantified variable
  typedef unsigned int TQuantifier;

  #define TYPE_BOOL  0
  #define TYPE_INT   1
  // Information on type of a variable
  typedef unsigned int TVarType;

  #define AND         0
  #define OR          1
  #define IMP         2
  #define XOR         3
  #define EQ          4
  #define TIMES       5
  #define LINEAR      6
  #define RE_AND      7
  #define RE_OR       8
  #define RE_IMP      9
  #define RE_XOR     10
  #define RE_EQ      11
  #define RE_TIMES   12
  #define RE_LINEAR  13
  #define ELEMENT    14
  // Information on type of a constraint
  typedef unsigned int TConstraintType;

  #define CMP_NQ  0
  #define CMP_EQ  1
  #define CMP_LQ  2
  #define CMP_LE  3
  #define CMP_GQ  4
  #define CMP_GR  5
  // Information on comparison operator
  typedef unsigned int TComparisonType;

  // Availables values types for a value.
  // Either a value (bool or int), an interval or the name of variable.
  #define VAL_NONE      0
  #define VAL_BOOL      1
  #define VAL_INT       2
  #define VAL_VAR       3
  #define VAL_INTERVAL  4
  #define VAL_CMP       5
  // Information on type of argument of a constraint
  typedef unsigned int TValType;

  struct TArg {
    TValType type;
    union {
      bool b;
      int  z;
      int  bounds[2];
    } val;
    std::string name;
    TComparisonType cmp;
    TArg() : type(VAL_NONE) {}
    TArg(bool _b) : type(VAL_BOOL) { val.b = _b; }
    TArg(int  _z) : type(VAL_INT) { val.z = _z; }
    TArg(std::string _name) : type(VAL_VAR), name(_name) {}
    TArg(const char *_name) : type(VAL_VAR), name(_name) {}
    TArg(int _min, int _max) : type(VAL_INTERVAL) { val.bounds[0] = _min; val.bounds[1] = _max; }
  };

  struct TArgCmp : TArg{
    TArgCmp(TComparisonType _cmp) : TArg() { type = VAL_CMP; cmp = _cmp; }
  };

  // Availables values types for a variable.
  struct TVal {
    TValType type;
    union {
      bool b;
      int  z;
      int  bounds[2];
    } val;
    TVal() : type(VAL_NONE) {}
    TVal(bool _b) : type(VAL_BOOL) { val.b = _b; }
    TVal(int  _z) : type(VAL_INT) { val.z = _z; }
    TVal(int _min, int _max) : type(VAL_INTERVAL) { val.bounds[0] = _min; val.bounds[1] = _max; }

    bool operator!=(const TVal& v) const { return !this->operator==(v); }
    bool operator==(const TVal& v) const {
      return ((v.type == type) && (
            (type == VAL_NONE) ||
            ((type == VAL_BOOL) && (val.b == v.val.b)) ||
            ((type == VAL_INT) && (val.z == v.val.z)) ||
            ((type == VAL_INTERVAL) && (val.bounds[0] == v.val.bounds[0]) && (val.bounds[1] == v.val.bounds[1]))
            ));
    }
  };
  typedef std::vector<TVal> TInstance;

  // Information given about a variable (Quantifier {ForAll, Exists}, Type (bool, int, etc ...), name, and id used)
  struct TVarInfo {
    bool inBinder;
    unsigned int id;
    TQuantifier  q;
    TVarType     t;
    std::string  name;
    TVal         dom;
    TVarInfo() {}
    TVarInfo(bool _inBinder, unsigned int _id, TQuantifier _q, const std::string& _name, TVarType _t, TVal _dom = TVal()) : inBinder(_inBinder), id(_id), q(_q), t(_t), name(_name), dom(_dom) {}
    bool operator!=(const TVarInfo& v) const { return !this->operator==(v); }
    bool operator==(const TVarInfo& v) const {
      return ((v.inBinder == inBinder) &&
              (v.id == id) &&
              (v.q == q) &&
              (v.t == t) &&
              (v.name == name) &&
              (v.dom == dom)
            );
    }
  };
  struct TVarBinder : TVarInfo {
    TVarBinder() : TVarInfo() {}
    TVarBinder(TQuantifier _q, const std::string& _name, TVarType _t, TVal _dom = TVal()) : TVarInfo(true, 0, _q, _name, _t, _dom) {}
  };
  struct TVarAux : TVarInfo {
    TVarAux() : TVarInfo() {}
    TVarAux(const std::string& _name, TVarType _t, TVal _dom = TVal()) : TVarInfo(false, 0, EXISTS, _name, _t, _dom) {}
  };

  // class forward;
  class SIBus;

  class SIBUS_VTABLE_EXPORT Receiver {
  protected:
    // Reference to the SIBus
    SIBus const * sibus;
  public:
    // Called when the process is set in sibus \a sibus.
    virtual void setSIBus(SIBus const * sibus);

    // Called when a new variables is received in the system
    virtual void eventVar(const TVarInfo& var) = 0;

    // Called when a new array of variables is received in the system
    virtual void eventVarArray(unsigned int size, const TVarInfo& var) = 0;

    // Called when the problem is modeled
    virtual void eventCloseModeling() = 0;

    // Called when a new constraint is received in the system.
    virtual void eventConstraint(TConstraintType type, const std::vector<TArg>& args) = 0;

    // All the following methods must be short in order to not
    // slow down the resolution process !!!

    // Called each time a choice has been made in the search tree.
    // \a idx the index of the chosen variables and \a val the chosen value
    virtual void eventChoice(int idx, TVal val) = 0;

    // Called each time a failure has been received.
    virtual void eventFailure() = 0;

    // Called when a global failure has been detected (end of the search).
    virtual void eventGlobalFailure() = 0;

    // Called each time a new instance has been sent.
    // \a instance gives the instance.
    virtual void eventInstance(const TInstance& instance) = 0;

    // Called when a success has been detected (end of the search).
    virtual void eventSuccess() = 0;

    // Called when a domain description is received by the system
    virtual void eventDomainDescription(unsigned int idVar, const std::vector<TVal>& domain) = 0;

    // Called when a swap of variables has been asked.
    virtual void eventSwapRequest(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) = 0;

    // Called when a swap of variables has been done.
    virtual void eventSwapDone(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) = 0;

    // Virtual destructor
    virtual ~Receiver();
  };

  class SIBUS_VTABLE_EXPORT SIBus {
  public:
    // Main data structure which ensures data exchange between processes.
    struct TLEvent {
      TEvent e;
      unsigned int id;
      unsigned int idVal1;
      unsigned int idVal2;
      unsigned int size;
      bool swapError;
      TVarInfo var;
      TInstance instance;
      TVal val;
      TConstraintType cstType;
      std::vector<TVal> domain;
      std::vector<TArg> cstArgs;

      TLEvent(TEvent _e) : e(_e), id(0) { }
      TLEvent(TEvent _e, const TVarInfo& _var) : e(EV_VAR), var(_var) { assert(_e == EV_VAR); }
      TLEvent(TEvent _e, unsigned int _size, const TVarInfo& _var) : e(EV_VARARRAY), size(_size), var(_var) { assert(_e == EV_VARARRAY); }
      TLEvent(TEvent _e, unsigned int _id, const std::vector<TVal>& _domain) : e(EV_DOMAIN_DESC), id(_id), domain(_domain) { assert(_e == EV_DOMAIN_DESC); }
      TLEvent(TEvent _e, unsigned int _id, unsigned int _idVal1, unsigned int _idVal2) : e(_e), id(_id), idVal1(_idVal1), idVal2(_idVal2) { assert((_e == EV_SWAP_ASK) || (_e == EV_SWAP_DONE)); }
      TLEvent(TEvent _e, const TInstance& _instance) : e(EV_INSTANCE), instance(_instance) { assert(_e == EV_INSTANCE); }
      TLEvent(TEvent _e, unsigned int _id, TVal _val) : e(EV_CHOICE), id(_id), val(_val) { assert(_e == EV_CHOICE); }
      TLEvent(TEvent _e, TConstraintType _type, const std::vector<TArg>& _args) : e(EV_CONSTRAINT), cstType(_type), cstArgs(_args) { assert(_e == EV_CONSTRAINT); }
    };

  private:
    // Singleton instance
    static SIBus* m_pSIBusInstance;

    std::vector<Receiver*> receivers; // List of listeners.

#ifdef SIBUS_THREAD
    // Thread
    boost::thread * m_thread;
    // Mutex for access to sibus
    mutable BoostMutex mx;
    /// Event for push data in queue
    mutable BoostEvent ev_fifo;
    // List of events to be sent to receivers
    mutable std::list<TLEvent> m_fifo;
#endif

    // States of the SIBus (init, run, shutdown, off)
    static const unsigned int S_INIT     = 0;
    static const unsigned int S_RUN      = 1;
    static const unsigned int S_SHUTDOWN = 2;
    static const unsigned int S_OFF      = 3;
    unsigned int sibusState;

    // Size of an instance (number of variables of the instance)
    unsigned binderSize;

    // Copy constructor set private to disable it.
    SIBus(const SIBus&);
    // Main constructor set private to disable it.
    SIBus();

    // Manage an event
    inline void manageEvent(const TLEvent& e);

    // The function that whill be runned in the thread
    void run(void);

    public:
      // Static method to get the SIBus instance
      static SIBus& instance() { assert(m_pSIBusInstance); return *m_pSIBusInstance; }

      // Static method to create the SIBus instance
      static void create();

      // Static method to destroy the SIBus instance
      static void kill();

      // Add a new listener \a p to be informed on each event in the search tree.
      inline void addReceiver(Receiver& p);

      // Remove an existing receiver \a p from the list of listeners
      inline void removeReceiver(Receiver& p);

      // Add a new variable \a var.
      // If success, it returns true, false otherwise.
      inline bool sendVar(const TVarInfo& var);

      // Add a new array of \a size variables \a var.
      // If success, it returns true, false otherwise.
      inline bool sendVarArray(unsigned int size, const TVarInfo& var);

      // Close the modeling step.
      inline void sendCloseModeling();

      // Add a new constraint of type \a type with arguments \a args.
      inline void sendConstraint(TConstraintType type, const std::vector<TArg>& args);

      // Enable SIBus
      inline void setEnableEvent();

      // Disable SIBus
      inline void setDisableEvent();

      // Add a new choice of the search tree to the m_fifo data structure. \a idx gives
      // the index of the variable in the binder, and \a val the chosen value
      inline void sendChoice(int idx, TVal val);

      // Add a new failure discovered in the search tree to the m_fifo data structure.
      inline void sendFailure();

      // Ends the search and add a global failure event to the m_fifo data structure.
      inline void sendGlobalFailure();

      // Queue the current instance to the m_fifo data structure.
      inline void sendInstance(const TInstance& instance);

      // Ends the search and add a success event to the m_fifo data structure.
      inline void sendSuccess();

      // Send domain description of a variable (each index of the vector corresponds to the index
      // of the value in the domain)
      inline void sendDomainDescription(unsigned int idVar, const std::vector<TVal>& domain);

      // Ask a swap of two values \a idVal1 and \a idVal2 of variable \a idVar
      inline void sendSwapAsk(unsigned int idVar, unsigned int idVal1, unsigned int idVal2);

      // Send a swap done
      inline void sendSwapDone(unsigned int idVar, unsigned int idVal1, unsigned int idVal2);

      // Main destructor
      virtual ~SIBus();
  };

#include <sibus/sibus.hpp>

#endif
