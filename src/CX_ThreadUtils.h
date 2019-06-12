#pragma once

#include <mutex>

#include "ofEvent.h"
#include "ofEventUtils.h"

namespace CX {
namespace Util {

/*! This class models temporary ownership of the pointer to a mutex-protected object.

Many classes in CX must be designed with thread safety in mind. One aspect of thread
safety is making sure that shared variables are not being accessed from more than 1 thread
at a time. A mutex can be used to ensure mutally exclusive access to variables,
with the mutex being locked by the thread that is accessing the shared variables.

Given this setup, there is a use case that mutexes do not support easily: Class
member accessor functions that access mutex-protected shared variables.

Usage:
\code{.cpp}
// Somewhere in code, set up shared data, a mutex (can be std::recursive_mutex), 
// and a function to safely access the shared data.
std::vector<int> threadData; // Accessed from multiple threads
std::mutex dataMutex; // Used to lock threadData while its being accessed

Util::LockedPointer<std::vector<int>, std::mutex> getLockedDataPointer(void) {
	// Construct a LockedPointer and return it. The constructor locks the mutex.
	return Util::LockedPointer<std::vector<int>, std::mutex>(&threadData, dataMutex);
}


// Somewhere else in code, getLockedDataPointer() can be used.
// Notice that this part of the code only needs to know 
// about getLockedDataPointer(), not the underlying data and mutex.

void someFunction(void) {
	auto ldp = getLockedDataPointer(); // It's legit to use auto for a type as long as the return type of getLockedDataPointer().

	// operator-> lets you treat the LockedPointer as a normal pointer.
	size_t n = ldp->size(); // Check the size of the vector.

	// Use get() to get the pointer
	std::vector<int>* dp = ldp.get();
	// do stuff with dp

	// When ldp falls out of scope and is destructed, it unlocks the mutex.
}

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
		return _default; // lool haxx0rz
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

	LockedPointer<ObjType, MutexType> getLockedPointer(void) {
		if (_lock.owns_lock()) {
			return
		}
	}
};


template <typename DataType, typename MutexType = std::recursive_mutex>
class ThreadsafeObject {
public:

	ThreadsafeObject(void) {}

	ThreadsafeObject(DataType d) :
		_data(d)
	{}

	template <typename MT>
	ThreadsafeObject(ThreadsafeObject<DataType, MT>& other) {
		this->operator=<MT>(other);
	}

	void set(DataType d) {
		_mutex.lock();
		_data = d;
		_mutex.unlock();
	}

	DataType get(void) {
		_mutex.lock();
		DataType rval = _data;
		_mutex.unlock();
		return rval;
	}

	LockedPointer<DataType, MutexType> getLockedPointer(void) {
		return LockedPointer<DataType, MutexType>(&_data, _mutex);
	}

	template <typename MT>
	ThreadsafeObject& operator=(ThreadsafeObject<DataType, MT>& rhs) {
		// Lock both mutexes in non-deadlocking way
		std::unique_lock<MutexType> lhsLock(this->_mutex, std::defer_lock);
		std::unique_lock<MutexType> rhsLock(rhs._mutex, std::defer_lock);

		std::lock(lhsLock, rhsLock);

		this->_data = rhs._data;

		return *this;
	}

	ThreadsafeObject& operator=(ThreadsafeObject<DataType, MutexType>& rhs) {
		return this->operator=<MutexType>(rhs);
	}

private:

	MutexType _mutex;
	DataType _data;

};


} // namespace Util
} // namespace CX