#pragma once
#include "MyDeque.h"
#include "MyList.h"

template <typename T, typename Container = MyDeque<T>>
class MyStack {	
public:
	MyStack(int n) :MAXNUM{ n }, element{ Container(n) } {}
	void pop();
	void push(T);
	T& top();
	bool empty();
private:
	int MAXNUM;
	Container element;
};

template<typename T, typename Container>
inline void MyStack<T, Container>::pop()
{
	if (element.empty()) throw runtime_error("empty!!!!");
	else element.pop_back();
}

template<typename T, typename Container>
inline void MyStack<T, Container>::push(T e)
{
	if (element.size() == MAXNUM) throw runtime_error("full!!!!");
	else {
		element.push_back(e);
	}
}

template<typename T, typename Container>
inline T & MyStack<T, Container>::top()
{
	if (element.empty()) throw runtime_error("empty!!!!");
	else return element.back();
}

template<typename T, typename Container>
inline bool MyStack<T,Container>::empty()
{
	return element.empty();
}
