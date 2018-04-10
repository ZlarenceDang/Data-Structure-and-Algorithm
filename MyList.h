#pragma once
#include <initializer_list>
#include <stdexcept>
template<typename T>
class MyList {
private:
	struct Node {
		T t;
		Node* pre;
		Node* next;
		Node(T tVal = T(), Node* preVal = NULL, Node* nextVal = NULL) :t{ tVal }, pre{ preVal }, next{ nextVal } {}
	};
public:
	class const_iterator {
	public:
		const_iterator() :current{ NULL } {}
		const T& operator*() const { return retrieve(); }
		const_iterator& operator++() { current = current->next; return *this; }
		const_iterator& operator++(int) {
			const_iterator old = *this;
			current = current->next;
			return old;
		}
		const_iterator& operator--() { current = current->pre; return *this; }
		const_iterator& operator--(int) {
			const_iterator old = *this;
			current = current->pre;
			return old;
		}
		bool operator==(const_iterator& rhs) const { return current == rhs.current; }
		bool operator!=(const_iterator& rhs) const { return current != rhs.current; }

	protected:
		Node* current;
		T& retrieve() const{ return current->t; }
		const_iterator(Node* p) :current{ p } {}
		friend class MyList<T>;
	};

	class iterator : public const_iterator {
	public:
		iterator() :current{ NULL } {}
		T& operator*() { return retrieve(); }
		const T& operator*() const { return const_iterator::operator*(); }
		iterator& operator++() { current = current->next; return *this; }
		iterator& operator++(int) {
			iterator old = *this;
			current = current->next;
			return old;
		}
		iterator& operator--() { current = current->pre; return *this; }
		iterator& operator--(int) {
			iterator old = *this;
			current = current->pre;
			return old;
		}

	protected:
		iterator(Node* p) { current = p; }
		T& retrieve() { return current->t; }
		friend class MyList<T>;
	};


public:
	MyList() { init(); }
	explicit MyList(int n) { init(); }
	MyList(const MyList& rhs) {	
		init();
		*this = rhs;
	}
	explicit MyList(int n, const T& value) {
		init();
		for (int i = 0; i < n; i++) push_back(value);
	}
	MyList(std::initializer_list<T> il) {
		init();
		for (auto it = il.begin(); it != il.end(); it++) push_back(*it);
	}

	~MyList() {
		clear();
		delete head;
		delete tail;
	}

	MyList& operator=(const MyList& rhs) {
		clear();
		for (iterator it = rhs.begin(); it != rhs.end(); it++) push_back(*it);
	}

	iterator begin() { return iterator(head->next); }
	const_iterator begin() const { return const_iterator(head->next); }
	iterator end() { return iterator(tail); }
	const_iterator end() const { return const_iterator(tail); }

	void clear() { while (theSize > 0) pop_back(); }

	void pop_back() {
		if (theSize == 0) throw std::runtime_error("MyList::empty!!!!");
		else {
			Node* p = tail->pre;
			p->pre->next = tail;
			tail->pre = p->pre;
			delete p;
			theSize--;
		}
	}

	void pop_front() {
		if (theSize == 0) throw std::runtime_error("MyList::empty!!!!");
		else {
			Node* p = head->next;
			p->next->pre = head;
			head->next = p->next;
			delete p;
			theSize--;
		}
	}

	void push_back(T t) {
		Node* p = new Node(t);
		tail->pre->next = p;
		p->pre = tail->pre;
		tail->pre = p;
		p->next = tail;
		theSize++;
	}
	void push_front(T) {
		Node* p = new Node(t);
		head->next->pre = p;
		p->next = head->next;
		head->next = p;
		p->pre = head;
		theSize++;
	}

	//insert befor it, it can be MyList::end()
	void insert(iterator it, T t) {
		if (it != end() && !it) throw std::runtime_error("MyList::insert before NULL!!!!");
		Node* p = new Node(t);
		it.current->pre->next = p;
		p->pre = it.current->pre;
		it.current->pre = p;
		p->next = it.current;
		theSize++;
	}

	void erase(iterator it) {
		if (!it) throw std::runtime_error("MyList::erase NULL!!!!");
		Node* p = it.current;
		p->pre->next = p->next;
		p->next->pre = p->pre;
		delete p;
		theSize--;
	}

	T& front() { return *(begin()); }
	T& back() { return *(--end()); }

	bool empty() const { return theSize == 0; }
	int size() const { return theSize; }

private:
	int theSize;
	Node* head;
	Node* tail;
	void init() {
		theSize = 0;
		head = new Node();
		tail = new Node();
		head->next = tail;
		tail->pre = head;
	}
};