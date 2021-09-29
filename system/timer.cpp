#include "timer.h"

#if TIMER_ON

uint64_t Timer::get_timestamp()
{
	return timestamp;
}

string Timer::get_hash()
{
	return hash;
}

Message *Timer::get_msg()
{
	return msg;
}

void Timer::set_data(uint64_t tst, string hsh, Message *cqry)
{
	timestamp = tst;
	hash = hsh;
	msg = cqry;
}

/************************************/

/* Start the timer for this transaction.
   To identify a txn we store the hash of the client request.	
*/
void ServerTimer::startTimer(string digest, Message *clqry)
{
	Timer *tmap = (Timer *)mem_allocator.alloc(sizeof(Timer));
	new (tmap) Timer();
	tmap->set_data(get_sys_clock(), digest, clqry);

	tlock.lock();
	txn_map.push_back(tmap);
	tlock.unlock();
}

/* When a txn completes, remove its timer. */
void ServerTimer::endTimer(string digest)
{
	Timer *tmap;
	tlock.lock();
	for (uint64_t i = 0; i < txn_map.size(); i++)
	{
		tmap = txn_map[i];
		if (tmap->get_hash() == digest)
		{
			txn_map.erase(txn_map.begin() + i);
			Message::release_message(tmap->get_msg());
			mem_allocator.free(tmap, sizeof(Timer));
			//delete tmap;
			break;
		}
	}
	tlock.unlock();
}

/* 
	We need to find all the client requests for which the timer has expired.
	Simplest way is to loop till first txn with difference between its 
	starting time and current time has not crossed the threshold. 
*/
bool ServerTimer::checkTimer()
{
	Timer *tmap;

	tlock.lock();
	// If timer is paused (true), then no further view changes.
	if (timer_state)
	{
		tlock.unlock();
		return false;
	}

	for (uint64_t i = 0; i < txn_map.size(); i++)
	{
		tmap = txn_map[i];
		if (get_sys_clock() - tmap->get_timestamp() < EXE_TIMEOUT)
		{
			break;
		}
		else
		{
			tlock.unlock();
			return true;
		}
	}

	tlock.unlock();
	return false;
}

void ServerTimer::pauseTimer()
{
	tlock.lock();
	timer_state = true;
	tlock.unlock();
}

void ServerTimer::resumeTimer()
{
	tlock.lock();
	timer_state = false;
	tlock.unlock();
}

/* Fetches the first entry */
Timer *ServerTimer::fetchPendingRequests(uint64_t idx)
{
	Timer *tmap = txn_map[idx];
	return tmap;
}

uint64_t ServerTimer::timerSize()
{
	return txn_map.size();
}

/* Clear all the timers. To be used at the end of a view change. */
void ServerTimer::removeAllTimers()
{
	Timer *tmap;
	uint64_t i = 0;
	tlock.lock();
	for (; i < txn_map.size();)
	{
		tmap = txn_map[i];
		txn_map.erase(txn_map.begin() + i);
		Message::release_message(tmap->get_msg());
		mem_allocator.free(tmap, sizeof(Timer));
		//delete tmap;
	}
	txn_map.clear();
	tlock.unlock();
}

/************************************/

/* 
	Start the timer for this transaction.
	To identify the transmitted batch we store the timestamp of the first request.
	Further, as we do not require hash for this empty, so we store random string.
*/
void ClientTimer::startTimer(uint64_t timestp, ClientQueryBatch *cqry)
{
	Timer *tmap = (Timer *)mem_allocator.alloc(sizeof(Timer));
	new (tmap) Timer();
	tmap->set_data(timestp, "A", cqry);

	tlock.lock();
	txn_map.push_back(tmap);
	// printf("added  %ld  %ld\n",cqry->txn_id, txn_map.size());
	tlock.unlock();
}

/* When a txn completes, remove its timer. */
void ClientTimer::endTimer(uint64_t timestp)
{
	Timer *tmap;
	tlock.lock();
	for (uint64_t i = 0; i < txn_map.size(); i++)
	{
		tmap = txn_map[i];
		if (tmap->get_timestamp() == timestp)
		{
			txn_map.erase(txn_map.begin() + i);
			Message::release_message(tmap->get_msg());
			mem_allocator.free(tmap, sizeof(Timer));
			// printf("remvd  %ld  %ld\n",tmap->get_msg()->txn_id, i);
			break;
		}
	}
	tlock.unlock();
}

/*
	We need to find all the client requests for which the timer has expired.
	Simplest way is to loop till first txn with difference between its 
	starting time and current time has not crossed the threshold.
*/
bool ClientTimer::checkTimer(ClientQueryBatch *&cbatch)
{
	Timer *tmap;
	bool flag = false;

	tlock.lock();
	for (uint64_t i = 0; i < txn_map.size(); i++)
	{
		tmap = txn_map[i];
		if (get_sys_clock() - tmap->get_timestamp() < CEXE_TIMEOUT)
		{
			break;
		}
		else
		{
			// Create a copy of this client batch.
			char *buf = create_msg_buffer(tmap->get_msg());
			cbatch = (ClientQueryBatch *)deep_copy_msg(buf, tmap->get_msg());
			delete_msg_buffer(buf);

			// Delete this entry from the timer.
			txn_map.erase(txn_map.begin() + i);
			Message::release_message(tmap->get_msg());
			mem_allocator.free(tmap, sizeof(Timer));

			// Found so return true.
			flag = true;
			printf("resending to primary  %ld\n",tmap->get_msg()->txn_id);
		}
	}

	tlock.unlock();
	return flag;
}

/* Fetches the first entry */
Timer *ClientTimer::fetchPendingRequests()
{
	Timer *tmap = txn_map[0];
	return tmap;
}

/************************************/

/* Timers */

ServerTimer *server_timer;
ClientTimer *client_timer;
std::mutex tlock;

#endif
