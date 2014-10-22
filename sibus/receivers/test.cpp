#include <iostream>
#include "receiver-compil-certif.hh"

int main() {
  Tree<int> * t = new Tree<int>;
  std::list<int> l;
  l.push_back(0);
  l.push_back(1);
  t->add(l);

  l.clear();
  l.push_back(0);
  l.push_back(2);
  t->add(l);

  l.clear();
  l.push_back(0);
  l.push_back(3);
  t->add(l);

  l.clear();
  l.push_back(0);
  l.push_back(1);
  l.push_back(4);
  t->add(l);

  l.clear();
  l.push_back(0);
  l.push_back(1);
  l.push_back(5);
  t->add(l);

  l.clear();
  l.push_back(0);
  l.push_back(1);
  l.push_back(5);
  l.push_back(6);
  t->add(l);

  l.clear();
  l.push_back(0);
  l.push_back(3);
  l.push_back(7);
  t->add(l);

  Tree<int> t2(*t);
  l.clear();
  l.push_back(0);
  l.push_back(1);
  l.push_back(2);
  l.push_back(3);
  l.push_back(7);
  t2.add(l);
  l.clear();
  l.push_back(0);
  l.push_back(1);
  l.push_back(2);
  l.push_back(3);
  l.push_back(4);
  l.push_back(8);
  t2.add(l);

  std::cout << "t: ";
  Tree<int>::iterator it = t->begin();
  Tree<int>::iterator itEnd = t->end();
  for ( ; it != itEnd ; ++it)
    std::cout << *it << " ";
  std::cout << std::endl;

  std::cout << "Branches de t: " << std::endl;
  Tree<int>::BranchIterator itB = t->branchBegin();
  Tree<int>::BranchIterator itBEnd = t->branchEnd();
  for ( ; itB != itBEnd ; ++itB) {
    std::list<int>::const_iterator _it = (*itB).begin();
    std::list<int>::const_iterator _itEnd = (*itB).end();
    for ( ; _it != _itEnd ; ++_it)
      std::cout << *_it << " ";
    std::cout << std::endl;
  }
  std::cout << std::endl;

  delete t;
  t = NULL;

  std::cout << "t2: ";
  it = t2.begin();
  itEnd = t2.end();
  for ( ; it != itEnd ; ++it)
    std::cout << *it << " ";
  std::cout << std::endl;

  std::cout << "Branches de t2: " << std::endl;
  itB = t2.branchBegin();
  itBEnd = t2.branchEnd();
  for ( ; itB != itBEnd ; ++itB) {
    std::list<int>::const_iterator _it = (*itB).begin();
    std::list<int>::const_iterator _itEnd = (*itB).end();
    for ( ; _it != _itEnd ; ++_it)
      std::cout << *_it << " ";
    std::cout << std::endl;
  }
  std::cout << std::endl;
  return 0;
}

