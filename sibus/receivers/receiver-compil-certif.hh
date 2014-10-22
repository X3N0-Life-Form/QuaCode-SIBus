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

#ifndef __RECEIVER_COMPIL_CERTIF_HH__
#define __RECEIVER_COMPIL_CERTIF_HH__

#include <sibus/sibus.hh>
#include <vector>
#include <stack>
#include <list>

// Simple class to implement trees
template <class T >
class Tree {
  class Node {
    friend class Tree<T>;
    T val;
    std::list< Node* > childs;
  };
  // Root node
  Node * mRoot;

  // Number of nodes in tree
  unsigned int mNbNodes;

  // Assignment operator set private to avoid it
  Tree& operator=(const Tree&);

  public:
		Tree() : mRoot(NULL), mNbNodes(0) {}
    Tree(const Tree& other) : mRoot(NULL), mNbNodes(other.mNbNodes) {
      if (other.mRoot == NULL) return;
      std::stack< Node* > _stack, _stackOther;

      mRoot = new Node;
      mRoot->val = other.mRoot->val;
      _stack.push(mRoot);
      _stackOther.push(other.mRoot);

      while (!_stackOther.empty()) {
        Node * n, * nOther;
        n = _stack.top();
        _stack.pop();
        nOther = _stackOther.top();
        _stackOther.pop();
        typename std::list< Node* >::iterator it    = nOther->childs.begin();
        typename std::list< Node* >::iterator itEnd = nOther->childs.end();
        for ( ; it != itEnd ; ++it ) {
          Node * _n = new Node;
          _n->val = (*it)->val;
          n->childs.push_back(_n);
          _stack.push(_n);
          _stackOther.push(*it);
        }
      }
    }

    bool empty() const { return mRoot == NULL; }
    unsigned int nodes() const { return mNbNodes; }

    ~Tree() {
      Node * n = NULL;
      PreOrderIterator it    = begin();
      PreOrderIterator itEnd = end();
      while (it != itEnd) {
        n = it._mNode;
        ++it;
        delete n;
        mNbNodes--;
      }
    }

    class PreOrderIterator {
      friend class Tree<T>;
      Node * _mNode;
      std::stack< Node* > _mStack;
      public:
        PreOrderIterator() : _mNode(NULL) {}
        PreOrderIterator(Node * node) : _mNode(node) {}

        T& operator*() const { return _mNode->val; }

        bool operator==(const PreOrderIterator& other) const { return (other._mNode == _mNode); }
        bool operator!=(const PreOrderIterator& other) const { return (other._mNode != _mNode); }
        PreOrderIterator&  operator++() {
          if (!_mNode->childs.empty()) {
            typename std::list< Node* >::reverse_iterator it    = _mNode->childs.rbegin();
            typename std::list< Node* >::reverse_iterator itEnd = _mNode->childs.rend();
            for ( ; it != itEnd ; ++ it ) _mStack.push(*it);
          }
          if (_mStack.empty())
            _mNode = NULL;
          else {
            _mNode = _mStack.top();
            _mStack.pop();
          }
          return *this;
        }
		};
		typedef PreOrderIterator iterator;
		PreOrderIterator begin() const { return PreOrderIterator(mRoot); }
		PreOrderIterator end() const { return PreOrderIterator(); }


    class BranchIterator {
      friend class Tree<T>;
      typedef typename std::list<Node*>::iterator NodeListIterator;
      Node * _mNode;
      std::stack< std::pair<NodeListIterator,NodeListIterator> > _mStack;
      std::list< T > _branch;
      public:
        BranchIterator() : _mNode(NULL) {}
        BranchIterator(Node * node) : _mNode(node) {
          if (node == NULL) return;
          Node * n = node;
          _branch.push_back(n->val);
          while (!n->childs.empty()) {
            // Iterator different from End because !n->childs.empty()
            _mStack.push( std::pair<NodeListIterator, NodeListIterator>(n->childs.begin(), n->childs.end()));
            n = *(_mStack.top().first);
            _branch.push_back(n->val);
          }
          _mNode = n;
        }

        const std::list<T>& operator*() const { return _branch; }

        bool operator==(const BranchIterator& other) const { return (other._mNode == _mNode); }
        bool operator!=(const BranchIterator& other) const { return (other._mNode != _mNode); }
        BranchIterator&  operator++() {
          if (!_mStack.empty()) {
            // We backtrack
            NodeListIterator* nli = &_mStack.top().first;
            NodeListIterator* nliEnd = &_mStack.top().second;
            ++(*nli); // If iterator is in stack it is not at the end
            _branch.pop_back();
            while (*nli == *nliEnd) {
              _mStack.pop();
              _branch.pop_back();
              if (_mStack.empty()) break;
              nli = &_mStack.top().first;
              nliEnd = &_mStack.top().second;
              ++(*nli); // If iterator is in stack it is not at the end
            }

            if (!_mStack.empty()) {
              // We follow next branch
              Node * n = **nli;
              _branch.push_back(n->val);
              while (!n->childs.empty()) {
                // Iterator different from End because !n->childs.empty()
                _mStack.push( std::pair<NodeListIterator, NodeListIterator>(n->childs.begin(), n->childs.end()));
                n = *(_mStack.top().first);
                _branch.push_back(n->val);
              }
              _mNode = n;
            } else {
              _mNode = NULL;
              _branch.clear();
            }
          } else {
            _mNode = NULL;
            _branch.clear();
          }
          return *this;
        }
		};
		BranchIterator branchBegin() const { return BranchIterator(mRoot); }
		BranchIterator branchEnd() const { return BranchIterator(); }

    // Add branch of values to the tree
    void add(const std::list<T>& values) {
      if (values.empty()) return;

      Node * n = mRoot;
      typename std::list<T>::const_iterator itV    = values.begin();
      typename std::list<T>::const_iterator itVEnd = values.end();
      // Check if part of the branch exists in the tree
      if (mRoot != NULL) {
        if (mRoot->val != *itV) return; // Root are not equal we can't add the branch
        ++itV; // We know that list is not empty because it was tested earlier
        for ( ; itV != itVEnd ; ++itV) {
          typename std::list< Node* >::iterator itC    = n->childs.begin();
          typename std::list< Node* >::iterator itCEnd = n->childs.end();
          for ( ; itC != itCEnd ; ++itC)
            if ((*itC)->val == *itV) {
              n = *itC;
              break;
            }
          if (itC == itCEnd) break;
        }
      }

      // Create nodes to complete the branch
      for ( ; itV != itVEnd ; ++itV) {
        Node * _n = new Node;
        mNbNodes++;
        _n->val = *itV;
        if (mRoot == NULL) mRoot = _n;
        else n->childs.push_back(_n);
        n = _n;
      }
    }
};

// Type for a guard element
struct TGuardElt {
  TVarInfo v;
  Tree<TVarInfo> a;
  TGuardElt(TVarInfo _v, Tree<TVarInfo> _a) : v(_v), a(_a) {}
};

// Type for a guard which is a list of TGuardElt
typedef std::list<TGuardElt> TGuard;

// Type for a value associated with a list of guards
struct TVGuard {
  TVarInfo x;
  TGuard g;
  TVGuard(TVarInfo _x) : x(_x) {}
};

class SIBUS_VTABLE_EXPORT ProcessCOMPIL : public Receiver {
  // The Binder of the problem
  std::vector<TVarInfo> binder;
  // Auxiliary variables of the problem
  std::vector<TVarInfo> varAux;

  // Current wining scenario
  typedef std::list<TVarInfo> TScenario;
  TScenario scenario;
  // Stock of winning scenario
  std::list<TScenario> lsc;

  // Boolean flag to know if we create guards from instance or
  // from search tree (so modulo the propagation effort)
  bool bModuloProp;

  // Boolean flag to know if we can from a failure
  bool failure;
  // Copy of current scenario when last failur occured
  // Usefull to remove irrelevant scenario from the list of scenarios 
  TScenario failedScenario;
  // Boolean flag to know if we can from an instance
  bool instance;

public:
  ProcessCOMPIL(bool _bModuloProp = false);
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

  virtual ~ProcessCOMPIL();

  void insertScenario(TScenario sc, std::list<TVGuard>& guards);
};

#endif
