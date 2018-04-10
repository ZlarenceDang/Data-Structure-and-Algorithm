#pragma once


template <typename T>
class MyDeque {
public:
	MyDeque(int n) :MAXNUM{ n }, head{ -1 }, tail{ -1 }, element{ new T[n] } {}
	void pop_back();
	void pop_front();
	void push_back(T);
	void push_front(T);
	T& back();
	T& front();

	bool empty();
	int size();
private:
	int MAXNUM;
	int head, tail;
	T* element;
};


template<typename T>
void MyDeque<T>::pop_back()
{
	if (tail < 0) throw runtime_error("empty!!!!");
	else {
		if (tail > head) tail--;
		else if (tail == head) { tail = -1; head = -1; }
		else if (tail < head && tail >0) { tail--; }
		else if (tail < head && tail == 0) { tail = MAXNUM - 1; }
	}
}

template<typename T>
void MyDeque<T>::pop_front()
{
	if (head < 0) throw runtime_error("empty!!!!");
	else {
		if (tail > head) head++;
		else if (tail == head) { tail = -1; head = -1; }
		else if (tail < head && head < MAXNUM - 1) { head++; }
		else if (tail < head && head == MAXNUM - 1) { head = 0; }
	}
}

template<typename T>
void MyDeque<T>::push_back(T e)
{
	if ((head > 0 && tail == head - 1) || (head == 0 && tail == MAXNUM - 1)) throw runtime_error("full!!!!");
	else {
		if (empty()) { head = 0; tail = 0; element[tail] = e; }
		else if (tail < MAXNUM - 1) { tail++; element[tail] = e; }
		else if (tail == MAXNUM - 1) { tail = 0; element[tail] = e; }
	}
}

template<typename T>
void MyDeque<T>::push_front(T e)
{
	if ((head > 0 && tail == head - 1) || (head == 0 && tail == MAXNUM - 1)) throw runtime_error("full!!!!");
	else {
		if (empty()) { head = 0; tail = 0; element[tail] = e; }
		else if (head > 0) { head--; element[head] = e; }
		else if (head == 0) { head = MAXNUM - 1; element[head] = e; }
	}
}

template<typename T>
T & MyDeque<T>::back()
{
	return element[tail];
}

template<typename T>
T & MyDeque<T>::front()
{
	return element[head];
}

template<typename T>
bool MyDeque<T>::empty()
{
	if (tail == -1 || head == -1) return true;
	else return false;
}

template<typename T>
int MyDeque<T>::size()
{
	if (empty()) return 0;
	else if (tail >= 0 && tail >= 0) {
		if (tail >= head) return (tail - head + 1);
		else return (tail + MAXNUM - head + 1);
	}
	else throw runtime_error("unknown error!!!!");
}