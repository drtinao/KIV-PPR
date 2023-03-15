#include "Watchdog.h"
#include "const.h"

/*
Acts as a singleton, only one watchdog needed.
*/
Watchdog* Watchdog::get_instance()
{
	static Watchdog *instance = new Watchdog(WATCHDOG_TIMEOUT_MS);
	return instance;
}

/*
Private constructor expects just timeout value in timer_ms. If timeout is exceeded, watchdog starts to warn user using console messages.
int timer_ms = timeout in timer_ms
*/
Watchdog::Watchdog(int timer_ms)
{
	this->timer_ms = std::chrono::milliseconds(timer_ms);
	this->latest_reset_time = std::chrono::high_resolution_clock::now();
	this->dog_active = false;
}

/*
Functions starts watchdog in separate thread (if not yet activated).
*/
void Watchdog::start_watchdog()
{
	std::unique_lock<std::mutex> uniq_mutex(dog_mutex);
	if (this->dog_active) {
		return; //watchdog already activated
	}
	else if (this->timer_ms == std::chrono::milliseconds(0)) {
		return; //0 ms is not reasonable in this application
	}
	else { //all ok, start watchdog
		this->dog_thread = std::thread(&Watchdog::watch_loop, this);
		this->dog_active = true;
		std::cout << "Watchdog started, reset timeout is " << WATCHDOG_TIMEOUT_MS << " ms." << std::endl;
	}
}

/*
Stops watchdog from app watching (if already active, else does nothing).
*/
void Watchdog::stop_watchdog()
{
	std::unique_lock<std::mutex>(dog_mutex);
	if (!this->dog_active) {
		return; //watchdog already disabled
	}
	else { //disable watchdog, nicely dispose thread
		this->dog_active = false;
		if (this->dog_thread.joinable()) {
			this->dog_thread.join();
		}
		std::cout << "Watchdog operation finished, all ok." << std::endl;
	}
}

/*
Function resets watchdog timer - called from other threads.
*/
void Watchdog::reset_timer()
{
	std::unique_lock<std::mutex>(dog_mutex);
	this->latest_reset_time = std::chrono::high_resolution_clock::now();
}

/*
Action periodically performed by watchdog thread.
*/
void Watchdog::watch_loop() {
	std::unique_lock<std::mutex>(dog_mutex);
	while (this->dog_active) {
		if (std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()) >= std::chrono::time_point_cast<std::chrono::milliseconds>(this->latest_reset_time) + this->timer_ms) {
			std::cout << "Watchdog did not received reset signal and timeout occured. Please restart app, else results may be invalid..." << std::endl;
		}
	}
}