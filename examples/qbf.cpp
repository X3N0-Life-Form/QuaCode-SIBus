/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Vincent Barichard <Vincent.Barichard@univ-angers.fr>
 *
 *  Copyright:
 *     Vincent Barichard, 2013
 *
 *  Last modified:
 *     $Date$ by $Author$
 *     $Revision$
 *
 *  This file is part of Quacode:
 *     http://quacode.barichard.com
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
#include <vector>
#include <map>

#include <quacode/qspaceinfo.hh>
#include <gecode/minimodel.hh>
#include <gecode/driver.hh>

#include <sibus/receivers/receiver-out.hh>
#include <sibus/receivers/receiver-nodecount.hh>
#include <sibus/receivers/receiver-compil-certif.hh>

using namespace Gecode;


#ifdef GECODE_HAS_GIST
namespace Gecode { namespace Driver {
  /// Specialization for QDFS
  template<typename S>
  class GistEngine<QDFS<S> > {
  public:
    static void explore(S* root, const Gist::Options& opt) {
      (void) Gist::explore(root, false, opt);
    }
  };
}}
#endif

// Struct utile pour simplifier la syntaxe de l'initialisation des vectors par des listes
template <typename T>
struct vlist_of : public std::vector<T> {
  vlist_of(const T& t) {
    (*this)(t);
  }
  vlist_of& operator()(const T& t) {
    this->push_back(t);
    return *this;
  }
};

/**
 * \brief Options taking one additional parameter
 */
class QBFOptions : public Options {
  Driver::BoolOption _qConstraint; /// Parameter to decide between optimized quantified constraints or usual ones
public:
  int n; /// Parameter to be given on the command line
  /// Initialize options for example with name \a s
  QBFOptions(const char* s, int n0, bool _qConstraint0)
    : Options(s),
      _qConstraint("-quantifiedConstraints",
                   "whether to use quantified optimized constraints",
                   _qConstraint0),
      n(n0)
  {
    add(_qConstraint);
  }
  /// Parse options from arguments \a argv (number is \a argc)
  void parse(int& argc, char* argv[]) {
    Options::parse(argc,argv);
    if (argc < 2)
      return;
    n = atoi(argv[1]);
  }
  bool qConstraint(void) const {
    return _qConstraint.value();
  }
  /// Print help message
  virtual void help(void) {
    Options::help();
    std::cerr << "\t(unsigned int) default: " << n << std::endl
              << "\t\tnumber of the instance used" << std::endl;
  }
};

class QBFProblem : public Script, public QSpaceInfo {
  BoolVarArray X;
public:
  QBFProblem(const QBFOptions& opt) : Script(), QSpaceInfo()
  {
    // DEBUT DESCRIPTION PB
    std::cout << "Loading problem" << std::endl;
    using namespace Int;

    X = BoolVarArray(*this,4,0,1);
    BoolVarArray O(*this,7,0,1);

    switch (opt.n) {
    case 0:
//    forall{a}(forall{b}(forall{c}(exists{d} ((c | b) & (b -> ((c -> d)&(c | (a <-> !d)))))))) OK UNSAT ( 6p,  4n, 1f) Q:( 0p,  0n, 1f)
      setForAll(*this,X[0]);
      setForAll(*this,X[1]);
      setForAll(*this,X[2]);
      SIBus::instance().sendVar(TVarBinder(FORALL,"a",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"b",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"c",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"d",TYPE_BOOL));
      break;
    case 1:
//    forall{a}(forall{b}(exists{c}(forall{d} ((c | b) & (b -> ((c -> d)&(c | (a <-> !d)))))))) OK UNSAT (27p, 10n, 2f) Q:( 3p,  0n, 1f)
      setForAll(*this,X[0]);
      setForAll(*this,X[1]);
      setForAll(*this,X[3]);
      SIBus::instance().sendVar(TVarBinder(FORALL,"a",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"b",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"c",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"d",TYPE_BOOL));
      break;
    case 2:
//    forall[a](exists[b](forall[c](forall[d] ((c | b) & (b -> ((c -> d)&(c | (a <-> !d)))))))) OK UNSAT (18p,  8n, 2f) Q:( 0p,  0n, 1f)
      setForAll(*this,X[0]);
      setForAll(*this,X[2]);
      setForAll(*this,X[3]);
      SIBus::instance().sendVar(TVarBinder(FORALL,"a",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"b",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"c",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"d",TYPE_BOOL));
      break;
    case 3:
//    forall{a}(exists{b}(forall{c}(exists{d} ((c | b) & (b -> ((c -> d)&(c | (a <-> !d)))))))) OK   SAT (40p, 13n, 2f) Q:(18p,  7n, 1f)
      setForAll(*this,X[0]);
      setForAll(*this,X[2]);
      SIBus::instance().sendVar(TVarBinder(FORALL,"a",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"b",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"c",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"d",TYPE_BOOL));
      break;
    case 4:
//    exists{a}(forall{b}(forall{c}(forall{d} ((c | b) & (b -> ((c -> d)&(c | (a <-> !d)))))))) OK UNSAT (18p,  9n, 2f) Q:( 0p,  0n, 1f)
      setForAll(*this,X[1]);
      setForAll(*this,X[2]);
      setForAll(*this,X[3]);
      SIBus::instance().sendVar(TVarBinder(EXISTS,"a",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"b",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"c",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"d",TYPE_BOOL));
      break;
    case 5:
//    exists{a}(forall{b}(exists{c}(forall{d} ((c | b) & (b -> ((c -> d)&(c | (a <-> !d)))))))) OK UNSAT (53p, 19n, 4f) Q:( 3p,  0n, 1f)
      setForAll(*this,X[1]);
      setForAll(*this,X[3]);
      SIBus::instance().sendVar(TVarBinder(EXISTS,"a",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"b",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"c",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"d",TYPE_BOOL));
      break;
    case 6:
//    exists{a}(exists{b}(exists{c}(forall{d} ((c | b) & (b -> ((c -> d)&(c | (a <-> !d)))))))) OK   SAT (16p,  5n, 0f) Q:( 9p,  5n, 0f)
      setForAll(*this,X[3]);
      SIBus::instance().sendVar(TVarBinder(EXISTS,"a",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"b",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"c",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(FORALL,"d",TYPE_BOOL));
      break;
    case 7:
//    exists[a](exists[b](exists[c](exists[d] ((c | b) & (b -> ((c -> d)&(c | (a <-> !d)))))))) OK   SAT ( 9p,  4n, 0f) Q:( 9p,  4n, 0f)
      SIBus::instance().sendVar(TVarBinder(EXISTS,"a",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"b",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"c",TYPE_BOOL));
      SIBus::instance().sendVar(TVarBinder(EXISTS,"d",TYPE_BOOL));
      break;
    default:
      GECODE_NEVER;
    }

    SIBus::instance().sendVar(TVarAux("o0",TYPE_BOOL));
    SIBus::instance().sendVar(TVarAux("o1",TYPE_BOOL));
    SIBus::instance().sendVar(TVarAux("o2",TYPE_BOOL));
    SIBus::instance().sendVar(TVarAux("o3",TYPE_BOOL));
    SIBus::instance().sendVar(TVarAux("o4",TYPE_BOOL));
    SIBus::instance().sendVar(TVarAux("o5",TYPE_BOOL));
    SIBus::instance().sendVar(TVarAux("o6",TYPE_BOOL));

    if (opt.qConstraint()) {
      QBoolVar qa, qb, qc, qd;
      qa = QBoolVar(quantifier(X[0]),X[0],1);
      qb = QBoolVar(quantifier(X[1]),X[1],2);
      qc = QBoolVar(quantifier(X[2]),X[2],3);
      qd = QBoolVar(quantifier(X[3]),X[3],4);
      qrel(*this, qa, BOT_XOR, qd, O[6]);
      qrel(*this, qc, BOT_OR, O[6], O[5]);
      qrel(*this, qc, BOT_IMP, qd, O[4]);
      qrel(*this, O[4], BOT_AND, O[5], O[3]);
      qrel(*this, qb, BOT_IMP, O[3], O[2]);
      qrel(*this, qc, BOT_OR, qb, O[1]);
      qrel(*this, O[1], BOT_AND, O[2], O[0]);
      rel(*this, O[0], IRT_EQ, 1);
    } else {
      BoolVar qa, qb, qc, qd;
      qa = X[0];
      qb = X[1];
      qc = X[2];
      qd = X[3];
      rel(*this, qa, BOT_XOR, qd, O[6]);
      rel(*this, qc, BOT_OR, O[6], O[5]);
      rel(*this, qc, BOT_IMP, qd, O[4]);
      rel(*this, O[4], BOT_AND, O[5], O[3]);
      rel(*this, qb, BOT_IMP, O[3], O[2]);
      rel(*this, qc, BOT_OR, qb, O[1]);
      rel(*this, O[1], BOT_AND, O[2], O[0]);
      rel(*this, O[0], IRT_EQ, 1);
    }

    branch(*this, X, INT_VAR_NONE(), INT_VAL_MIN());

    SIBus::instance().sendConstraint(XOR, vlist_of<TArg>("a")("d")("o6"));
    SIBus::instance().sendConstraint(OR , vlist_of<TArg>("c")("o6")("o5"));
    SIBus::instance().sendConstraint(IMP, vlist_of<TArg>("c")("d")("o4"));
    SIBus::instance().sendConstraint(AND, vlist_of<TArg>("o4")("o5")("o3"));
    SIBus::instance().sendConstraint(IMP, vlist_of<TArg>("b")("o3")("o2"));
    SIBus::instance().sendConstraint(OR , vlist_of<TArg>("c")("b")("o1"));
    SIBus::instance().sendConstraint(AND, vlist_of<TArg>("o1")("o2")("o0"));
    SIBus::instance().sendConstraint(EQ , vlist_of<TArg>("o0")(true));

    // FIN DESCRIPTION PB
    SIBus::instance().sendCloseModeling();
  }

  QBFProblem(bool share, QBFProblem& p) : Script(share,p), QSpaceInfo(*this,share,p)
  {
    X.update(*this,share,p.X);
  }

  virtual Space* copy(bool share) { return new QBFProblem(share,*this); }

  void eventNewInstance(void) const {
    TInstance instance;
    for (int i=0; i<X.size(); i++)
    {
      if (!X[i].varimp()->assigned())
        instance.push_back(TVal());
      else
        instance.push_back(((X[i].varimp()->one())?TVal(true):TVal(false)));
    }
    SIBus::instance().sendInstance(instance);
  }
  void print(std::ostream& os) const {
    strategyPrint(os);
  }
};

int main(int argc, char* argv[])
{
  ProcessSTDOUT pStdout;
  ProcessNodeCount pNodeCount;
  ProcessCOMPIL pCompil;

  SIBus::create();
  SIBus::instance().addReceiver(pStdout);
  SIBus::instance().addReceiver(pNodeCount);
  SIBus::instance().addReceiver(pCompil);

  QBFOptions opt("Non CNF, Non Prenex, Quantified Boolean Problem",0,true);
  opt.parse(argc,argv);
  Script::run<QBFProblem,QDFS,QBFOptions>(opt);

  SIBus::kill();

  return 0;
}

