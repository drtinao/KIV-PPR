#pragma once
#include <chrono>
#include <thread>
#include <mutex>
#include <iostream>

//contains utils related to program timer watchdog - for info about this watchdog concept see: https://en.wikipedia.org/wiki/Watchdog_timer.
class Watchdog
{
	static Watchdog* instance; //singleton instance of the dog

	private:
		bool dog_active; //tells whether watchdog is activated
		std::thread dog_thread; //thread reserved for watchdog
		std::mutex dog_mutex; //timer reset is performed from multiple threads, mutex required
		std::chrono::milliseconds timer_ms; //if timer runs out, watchdog will generate timeout signal (in this case just prints message to inform user...)
		std::chrono::steady_clock::time_point latest_reset_time; //latest time when was watchdog reseted
		Watchdog(int timer_ms); //constructor expects timeout value (in ms); private constructor, avoid more instances
		void watch_loop(); //action periodically performed by watching thread

	public:
		static Watchdog* get_instance(); //gets singleton instance
		void start_watchdog(); //performs watchdog activation
		void stop_watchdog(); //deactivates watchdog
		void reset_timer(); //resets timer
};