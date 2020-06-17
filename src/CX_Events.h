#pragma once

#include "ofEvents.h"

namespace CX {
namespace Util {

	/* Reduces the pain of using ofEvents, namely that listener classes must
	to stop listening to events when the listening class is destructed.
	ofEventHelper stops listening automatically when destructed.

	Sometimes this class just does not work, I don't know why.
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
			_currentEvent(nullptr),
			_stopEvent(nullptr)
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
			stopListening();
			setCallback(lfun);
			_listenTo(evp, priority);
		}

		template <class Listener>
		void setup(ofEvent<EvType>* evp, Listener* listener, std::function<void(Listener*, EvType)> cbMethod, int priority = (int)Priority::Normal) {
			stopListening();
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
			listenToStopEvent(nullptr, 0);
		}

		void listenToStopEvent(ofEvent<void>* sev, int priority = (int)Priority::Normal) {
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			if (_stopEvent) {
				ofRemoveListener(*_stopEvent, this, &ofEventHelper::stopListening, _stopPriority);
				_stopEvent = nullptr;
			}

			if (sev) {
				_stopEvent = sev;
				_stopPriority = priority;
				ofAddListener(*_stopEvent, this, &ofEventHelper::stopListening, _stopPriority);
			}
		}

	private:
		std::recursive_mutex _mutex;
		ofEvent<EvType>* _currentEvent;
		int _currentPriority;

		std::function<void(EvType)> _callback;

		ofEvent<void>* _stopEvent;
		int _stopPriority;

		inline void _listenFun(EvType t) {
			_mutex.lock();
			this->_callback(t);
			_mutex.unlock();
		}

		void _listenTo(ofEvent<EvType>* ev, int priority) {
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			if (_currentEvent) {
				_currentEvent->remove(this, &ofEventHelper::_listenFun, _currentPriority);

				//ofRemoveListener(*_currentEvent, this, &ofEventHelper::_listenFun, _currentPriority);
				_currentEvent = nullptr;
			}

			if (ev) {
				_currentEvent = ev;
				_currentPriority = priority;
				//ofAddListener(*_currentEvent, this, &ofEventHelper::_listenFun, _currentPriority);

				_currentEvent->add(this, &ofEventHelper::_listenFun, _currentPriority);
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
			_currentEvent(nullptr),
			_stopEvent(nullptr)
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
			stopListening();
			setCallback(lfun);
			_listenTo(evp, priority);
		}

		template <class Listener>
		void setup(ofEvent<void>* evp, Listener* listener, std::function<void(Listener*)> cbMethod, int priority = (int)Priority::Normal) {
			stopListening();
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
			listenToStopEvent(nullptr, 0);
		}

		void listenToStopEvent(ofEvent<void>* sev, int priority = (int)Priority::Normal) {
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			if (_stopEvent) {
				ofRemoveListener(*_stopEvent, this, &ofEventHelper::stopListening, _stopPriority);
				_stopEvent = nullptr;
			}

			if (sev) {
				_stopEvent = sev;
				_stopPriority = priority;
				ofAddListener(*_stopEvent, this, &ofEventHelper::stopListening, _stopPriority);
			}
		}

	private:
		std::recursive_mutex _mutex;
		ofEvent<void>* _currentEvent;
		int _currentPriority;

		std::function<void(void)> _callback;

		ofEvent<void>* _stopEvent;
		int _stopPriority;

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

} // namespace Util


namespace Private {

	class CX_Events {
	public:
		ofEvent<void> exitEvent;
	};

	CX_Events& getEvents(void);

}
}