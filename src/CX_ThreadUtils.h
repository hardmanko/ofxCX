#pragma once

#include <deque>
#include <mutex>
#include <atomic>

#include "ofEvent.h"
#include "ofEventUtils.h"

namespace CX {
namespace Util {

/*! This class models temporary ownership of the pointer to a mutex-protected object.

Usage:
\code{.cpp}
// Somewhere in code
std::vector<int> threadData; // Accessed from multiple threads
std::mutex dataMutex; // Used to lock threadData while its being accessed

Util::LockedPointer<std::vector<int>, std::mutex> getLockedDataPointer(void) {
	// Construct a LockedPointer and return it.
	return Util::LockedPointer<std::vector<int>, std::mutex>(&threadData, dataMutex);
}

// Somewhere else in code. Notice that this part of the code only needs to know 
// about getLockedDataPointer(), not the underlying data and mutex.
auto ldp = getLockedDataPointer(); // legit use of auto

// operator-> lets you treat the LockedPointer as a normal pointer.
size_t n = ldp->size();

int sum = 0;
for (size_t i = 0; i < n; i++) {
sum += (*ldp)[i]; // operator* dereferences pointer to get reference
}

// Use get() to get the pointer
std::vector<int>* ip = ldp.get();

\endcode
*/
template <class ObjType, class MutexType>
class LockedPointer {
public:

	LockedPointer(void) = delete; // no default construction

	LockedPointer(ObjType* obj, MutexType& m) :
		_pobj(obj)
	{
		_lock = std::unique_lock<MutexType>(m); // unique_lock releases the lock when it destructs
	}

	LockedPointer(ObjType* obj, MutexType& m, std::adopt_lock_t adopt) :
		_pobj(obj)
	{
		_lock = std::unique_lock<MutexType>(m, adopt);
	}

	ObjType* get(void) {
		if (_lock.owns_lock()) {
			return _pobj;
		}
		return nullptr;
	}

	ObjType* operator->(void) {
		return get();
	}

	ObjType& operator*(void) {
		return *get();
	}

	bool empty(void) {
		return !(_lock.owns_lock() && _pobj);
	}

	// After calling this function, this class instance is dead
	void releasePointer(void) {
		if (_lock.owns_lock()) {
			_lock.unlock();
			_lock.release();
			_pobj = nullptr;
		}
	}

protected:

	std::unique_lock<MutexType> _lock;
	ObjType* _pobj;

};

/*! Like LockedPointer, but can lock and unlock the mutex. 
Thus, this class is like a mutex that holds an object pointer. 
	
You can't copy this class, but you can std::move it.
*/
template <class ObjType, class MutexType>
class ManagedPointer : public LockedPointer<ObjType, MutexType> {
public:

	ManagedPointer(void) = delete; // no default construction

	ManagedPointer(ObjType* obj, MutexType& m) :
		LockedPointer(obj, m)
	{}

	ManagedPointer(ObjType* obj, MutexType& m, std::adopt_lock_t adopt) :
		LockedPointer(obj, m, adopt)
	{}


	void lock(void) {
		_lock.lock();
	}

	void unlock(void) {
		if (_lock.owns_lock()) {
			_lock.unlock();
		}
	}

	bool isLocked(void) {
		return _lock.owns_lock();
	}

	/*
	std::unique_lock<MutexType>& getLock(void) {
		return _lock;
	}
	*/
		
};

// this seems pointless
template <class ObjType, class MutexType>
class LockedReference {
public:

	LockedReference(void) = delete; // no default construction

	LockedReference(ObjType& obj, MutexType& m) :
		_robj(obj)
	{
		_lock = std::unique_lock<MutexType>(m); // unique_lock releases the lock when it destructs
	}

	LockedReference(ObjType& obj, MutexType& m, std::adopt_lock_t adopt) :
		_robj(obj)
	{
		_lock = std::unique_lock<MutexType>(m, adopt);
	}

	ObjType& get(void) {
		if (_lock.owns_lock()) {
			return _robj;
		}
		return _default;
	}

	operator ObjType&(void) {
		return get();
	}

	ObjType& operator->(void) {
		return get();
	}

	ObjType& operator*(void) {
		return get();
	}

	bool empty(void) = delete; // no empty
	void unlock(void) = delete; // no unlock: you can't null out a reference

protected:

	std::unique_lock<MutexType> _lock;
	ObjType& _robj;
	ObjType _default;

};


template <typename T>
class MessageQueue {
public:

	// push in one thread
	void push(const T& t) {
		_mutex.lock();
		_mq.push_back(t);
		_mutex.unlock();
	}

	// check if any are available and, if so, pop them one at a time
	// you may not pop in more than one thread, but you may push in more than one thread
	// multiple-producer, single-consumer
	size_t available(void) {
		_mutex.lock();
		size_t rval = _mq.size();
		_mutex.unlock();
		return rval;
	}

	T pop(void) {
		_mutex.lock();
		T copy = _mq.front();
		_mq.pop_front();
		_mutex.unlock();
		return copy;
	}

	// may only be called from the same thread as pop() and available()
	void clear(void) {
		_mutex.lock();
		_mq.clear();
		_mutex.unlock();
	}

private:
	std::recursive_mutex _mutex;
	std::deque<T> _mq;
};

template <>
class MessageQueue<void> {
public:

	MessageQueue(void);

	void push(void);

	size_t available(void);

	void pop(void);

	void clear(void);

private:
	std::atomic<size_t> _available;
};



template <typename T>
class PolledEventListener {
public:

	typedef std::remove_cv_t<std::remove_reference_t<T>> BaseType;

	PolledEventListener(void) :
		_prevEv(nullptr)
	{}

	PolledEventListener(ofEvent<T>* ev) {
		_listenTo(ev);
	}

	void listenTo(ofEvent<T>* ev) {
		_listenTo(ev);
	}

	size_t available(void) {
		return _mq.available();
	}

	BaseType pop(void) {
		return _mq.pop();
	}

	void clearEvents(void) {
		_mq.clear();
	}

private:
	MessageQueue<BaseType> _mq;
	ofEvent<T>* _prevEv;

	void _listenTo(ofEvent<T>* ev) {
		if (_prevEv) {
			ofRemoveListener(*_prevEv, this, &PolledEventListener::_listenFun);
			_prevEv = nullptr;
			this->clearEvents();
		}

		if (ev) {
			ofAddListener(*ev, this, &PolledEventListener::_listenFun);
			_prevEv = ev;
		}
	}

	void _listenFun(T t) {
		_mq.push(t);
	}
};

template <>
class PolledEventListener<void> {
public:

	PolledEventListener(void);
	PolledEventListener(ofEvent<void>* ev);

	void listenTo(ofEvent<void>* ev);

	size_t available(void);
	void pop(void);
	void clearEvents(void);

private:

	ofEvent<void>* _prevEv;

	std::atomic<size_t> _available;

	void _listenTo(ofEvent<void>* ev);
	void _listenFun(void);

};

/* Makes the pain of using ofEvents go away, namely the whole 
having to stop listening to events when the listening class is destructed.
ofEventHelper stops listening automatically when destructed.

*/
template <typename EvType>
class ofEventHelper {
public:

	enum class Priority : int {
		Early = 0,
		Normal = 100,
		Late = 200
	};

	ofEventHelper(void) :
		_currentEvent(nullptr)
	{}

	ofEventHelper(std::function<void(EvType)> listenFun) :
		ofEventHelper()
	{
		setCallback(listenFun);
	}

	template <class Listener>
	ofEventHelper(Listener* listener, std::function<void(Listener*, EvType)> listenerFun) :
		ofEventHelper()
	{
		setCallback<Listener>(listener, listenerFun);
	}

	ofEventHelper(ofEvent<EvType>* evp, std::function<void(EvType)> listenFun, int priority = (int)Priority::Normal) :
		ofEventHelper()
	{
		setup(evp, listenFun, priority);
	}

	template <class Listener>
	ofEventHelper(ofEvent<EvType>* evp, Listener* listener, std::function<void(Listener*, EvType)> listenerFun, int priority = (int)Priority::Normal) :
		ofEventHelper()
	{
		setup<Listener>(evp, listener, listenerFun, priority);
	}

	~ofEventHelper(void) {
		stopListening();
	}

	void setup(ofEvent<EvType>* evp, std::function<void(EvType)> lfun, int priority = (int)Priority::Normal) {
		setCallback(lfun);
		_listenTo(evp, priority);
	}

	template <class Listener>
	void setup(ofEvent<EvType>* evp, Listener* listener, std::function<void(Listener*, EvType)> cbMethod, int priority = (int)Priority::Normal) {
		setCallback<Listener>(listener, cbMethod);
		_listenTo(evp, priority);
	}

	void setCallback(std::function<void(EvType)> cb) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_callback = cb;
	}

	template <class Listener>
	void setCallback(Listener* listener, std::function<void(Listener*, EvType)> cbMethod) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_callback = std::bind(cbMethod, listener, std::placeholders::_1);
	}

	void listenTo(ofEvent<EvType>* evp, int priority = (int)Priority::Normal) {
		_listenTo(evp, priority);
	}

	bool isListening(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _currentEvent != nullptr;
	}

	void stopListening(void) {
		_listenTo(nullptr, 0); // priority doesn't matter if removing because _currentPriority is used to remove
	}

private:
	std::recursive_mutex _mutex;
	ofEvent<EvType>* _currentEvent;
	int _currentPriority;

	std::function<void(EvType)> _callback;

	inline void _listenFun(EvType t) {
		_mutex.lock();
		this->_callback(t);
		_mutex.unlock();
	}

	void _listenTo(ofEvent<EvType>* ev, int priority) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		if (_currentEvent) {
			ofRemoveListener(*_currentEvent, this, &ofEventHelper::_listenFun, _currentPriority);
			_currentEvent = nullptr;
		}

		if (ev) {
			_currentEvent = ev;
			_currentPriority = priority;
			ofAddListener(*_currentEvent, this, &ofEventHelper::_listenFun, _currentPriority);
		}
	}
	
};

template <>
class ofEventHelper<void> {
public:

	enum class Priority : int {
		Early = 0,
		Normal = 100,
		Late = 200
	};

	ofEventHelper(void) :
		_currentEvent(nullptr)
	{}

	ofEventHelper(std::function<void(void)> listenFun) :
		ofEventHelper()
	{
		setCallback(listenFun);
	}

	template <class Listener>
	ofEventHelper(Listener* listener, std::function<void(Listener*)> listenerFun) :
		ofEventHelper()
	{
		setCallback<Listener>(listener, listenerFun);
	}

	ofEventHelper(ofEvent<void>* evp, std::function<void(void)> listenFun, int priority = (int)Priority::Normal) :
		ofEventHelper()
	{
		setup(evp, listenFun, priority);
	}

	template <class Listener>
	ofEventHelper(ofEvent<void>* evp, Listener* listener, std::function<void(Listener*)> listenerFun, int priority = (int)Priority::Normal) :
		ofEventHelper()
	{
		setup<Listener>(evp, listener, listenerFun, priority);
	}

	~ofEventHelper(void) {
		stopListening();
	}

	void setup(ofEvent<void>* evp, std::function<void(void)> lfun, int priority = (int)Priority::Normal) {
		setCallback(lfun);
		_listenTo(evp, priority);
	}

	template <class Listener>
	void setup(ofEvent<void>* evp, Listener* listener, std::function<void(Listener*)> cbMethod, int priority = (int)Priority::Normal) {
		setCallback<Listener>(listener, cbMethod);
		_listenTo(evp, priority);
	}

	void setCallback(std::function<void(void)> cb) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_callback = cb;
	}

	template <class Listener>
	void setCallback(Listener* listener, std::function<void(Listener*)> cbMethod) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_callback = std::bind(cbMethod, listener);
	}

	void listenTo(ofEvent<void>* evp, int priority = (int)Priority::Normal) {
		_listenTo(evp, priority);
	}

	bool isListening(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _currentEvent != nullptr;
	}

	void stopListening(void) {
		_listenTo(nullptr, 0); // priority doesn't matter if removing because _currentPriority is used to remove
	}

private:
	std::recursive_mutex _mutex;
	ofEvent<void>* _currentEvent;
	int _currentPriority;

	std::function<void(void)> _callback;

	inline void _listenFun(void) {
		_mutex.lock();
		this->_callback();
		_mutex.unlock();
	}

	void _listenTo(ofEvent<void>* ev, int priority) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		if (_currentEvent) {
			ofRemoveListener(*_currentEvent, this, &ofEventHelper::_listenFun, _currentPriority);
			_currentEvent = nullptr;
		}

		if (ev) {
			_currentEvent = ev;
			_currentPriority = priority;
			ofAddListener(*_currentEvent, this, &ofEventHelper::_listenFun, _currentPriority);
		}
	}

};

}
}