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

#include <quacode/qspaceinfo.hh>
#include <gecode/minimodel.hh>
#include <gecode/driver.hh>

#include <sibus/receivers/receiver-out.hh>
#include <sibus/receivers/receiver-network.hh>
#include <sibus/receivers/receiver-gecode.hh>

ReceiverGecode rGecode;

// Struct utile pour simplifier la syntaxe de l'initialisation des vectors par des listes
template <typename T>
struct vlist_of : public std::vector<T> {
  vlist_of() { }
  vlist_of(const T& t) {
    (*this)(t);
  }
  vlist_of& operator()(const T& t) {
    this->push_back(t);
    return *this;
  }
  vlist_of& operator()(int t1, int t2) {
    this->push_back(TArg(t1,t2));
    return *this;
  }
};

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

/**
 * \brief Options taking one additional parameter
 */
class MatrixGameOptions : public Options {
protected:
  /// Optional file name
  Gecode::Driver::StringValueOption _file;
public:
  int n; /// Parameter to be given on the command line
  /// Initialize options for example with name \a s
  MatrixGameOptions(const char* s, int n0)
    : Options(s), _file("-file","File name for matrix input"), n(n0) {
    add(_file);
  }
  /// Parse options from arguments \a argv (number is \a argc)
  void parse(int& argc, char* argv[]) {
    Options::parse(argc,argv);
    if (argc < 2)
      return;
    n = atoi(argv[1]);
  }
  /// Print help message
  virtual void help(void) {
    Options::help();
    std::cerr << "\t(unsigned int) default: " << n << std::endl
              << "\t\tDepth of the matrix" << std::endl;
  }
  /// Return file name
  const char* file(void) const {
    return _file.value();
  }
};

class QCSPMatrixGame : public Script, public QSpaceInfo {
  IntVarArray X;

public:

  QCSPMatrixGame(const MatrixGameOptions& opt) : Script(), QSpaceInfo()
  {
    // DEBUT DESCRIPTION PB
    std::cout << "Loading problem" << std::endl;
    using namespace Int;
    int depth = opt.n;// Size of the matrix is 2^depth. Large values may take long to solve...
    int boardSize = (int)pow((double)2,(double)depth);
    std::srand(std::time(NULL));

    // If a file is given we take the matrix from the file
    bool bFile = false;
    std::vector<int> tab;
    if (opt.file() != NULL) {
      std::ifstream inS(opt.file());
      if(!inS.is_open())
        bFile = false;
      else {
        bFile = true;
        int x;
        while (inS >> x) tab.push_back(x);
        boardSize = (int)sqrt(tab.size());
        depth = (int) (log(boardSize) / log(2));
      }
    }

    IntArgs board(boardSize*boardSize);
    for (int i=0; i<boardSize; i++)
      for (int j=0; j<boardSize; j++)
        if (bFile)
          board[j*boardSize+i] = tab[j*boardSize+i];
        else
          board[j*boardSize+i] = (int)( (double)rand()  /  ((double)RAND_MAX + 1) * 50 ) < 25 ? 0:1;

    int nbDecisionVar = 2*depth;
    IntArgs access(nbDecisionVar);
    access[nbDecisionVar-1]=1;
    access[nbDecisionVar-2]=boardSize;
    for (int i=nbDecisionVar-3; i>=0; i--)
      access[i]=access[i+2]*2;

    // Print initial board
//    for (int i=0; i<boardSize; i++)
//    {
//        for (int j=0; j<boardSize; j++)
//          std::cout << board[i*boardSize+j] << " ";
//        std::cout << std::endl;
//    }
//    std::cout  << std::endl;

    // Defining the player variables
    IntVarArgs x;
    X = IntVarArray(*this,nbDecisionVar,0,1);
    for (int i=0; i<nbDecisionVar; i++)
    {
      if ((i%2) == 0) {
        std::stringstream ss_x; ss_x << "x" << i / 2;
        SIBus::instance().sendVar(TVarBinder(EXISTS,ss_x.str(),TYPE_INT,TVal(0,1)));
      } else {
        std::stringstream ss_y; ss_y << "y" << i / 2;
        SIBus::instance().sendVar(TVarBinder(FORALL,ss_y.str(),TYPE_INT,TVal(0,1)));
      }
      x << X[i];
      if ((i%2) == 1)
        setForAll(*this, X[i]);
    }

    // Goal constaints
    IntVar cstUn(*this,1,1);
    IntVar boardIdx(*this,0,boardSize*boardSize);
    linear(*this, access, x, IRT_EQ, boardIdx);
    element(*this, board, boardIdx, cstUn);
    SIBus::instance().sendVarArray(boardSize*boardSize,TVarAux("board",TYPE_INT,TVal(0,1)));
//    for (int i=0; i<board.size(); i++) {
//      std::stringstream ss_boardi; ss_boardi << "board[" << i << "]";
//      SIBus::instance().sendConstraint(EQ, vlist_of<TArg>(ss_boardi.str())(board[i]));
//    }
    SIBus::instance().sendVar(TVarAux("boardIdx",TYPE_INT,TVal(0,boardSize*boardSize-1)));
    vlist_of<TArg> l(TArgCmp(CMP_EQ));
    for (int i=0; i<nbDecisionVar; i++) {
      if ((i%2) == 0) {
        std::stringstream ss_x; ss_x << "x" << i / 2;
        l = l(access[i])(ss_x.str());
      } else {
        std::stringstream ss_y; ss_y << "y" << i / 2;
        l = l(access[i])(ss_y.str());
      }
    }
    l = l(-1)("boardIdx")(1)(0);
    SIBus::instance().sendConstraint(LINEAR, l);
    SIBus::instance().sendConstraint(ELEMENT, vlist_of<TArg>("board")("boardIdx")(1));

    // FIN DESCRIPTION PB
    branch(*this, rGecode, X, INT_VAR_NONE());
    SIBus::instance().sendCloseModeling();
  }

  QCSPMatrixGame(bool share, QCSPMatrixGame& p)
    : Script(share,p), QSpaceInfo(*this,share,p)
  {
    X.update(*this,share,p.X);
  }

  virtual Space* copy(bool share) { return new QCSPMatrixGame(share,*this); }

  void eventNewInstance(void) const {
    TInstance instance;
    for (int i=0; i<X.size(); i++)
    {
      if (!X[i].varimp()->assigned())
        instance.push_back(TVal());
      else
        instance.push_back(TVal(X[i].varimp()->val()));
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
  ProcessNetwork pNetwork;

  SIBus::create();
  SIBus::instance().addReceiver(pStdout);
  SIBus::instance().addReceiver(pNetwork);
  SIBus::instance().addReceiver(rGecode);

  MatrixGameOptions opt("QCSP Matrix-Game",5);
  opt.parse(argc,argv);
  Script::run<QCSPMatrixGame,QDFS,MatrixGameOptions>(opt);

  SIBus::kill();
  return 0;
}

