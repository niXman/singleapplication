
// Copyright (c) 2010-2016 niXman (i dot nixman dog gmail dot com). All
// rights reserved.
//
// This file is part of
//     SINGLEAPPLICATION(https://github.com/niXman/singleapplication) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <singleapplication/singleapplication.hpp>

#include <ctime>
#include <boost/thread.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

/**************************************************************************/

namespace single_application {

namespace bi = boost::interprocess;

struct single_application::impl {
	impl(single_application::main_function_ptr main, int argc, char **argv)
		:main_function(main)
		,argc(argc)
		,argv(argv)
		,named_mutex_name()
		,shared_object_name()
		,can_run(false)
		,thread_is_running(true)
	{
#ifdef WIN32
		static const std::string::value_type separator = '\\';
#else
		static const std::string::value_type separator = '/';
#endif
		const char *pos = std::strrchr(argv[0], separator);
		named_mutex_name = shared_object_name = ((!pos) ? argv[0] : (pos+1));
		named_mutex_name += "__singleapplication_named_mutex_";
		shared_object_name += "__singleapplication_shared_data_";

		bool already_running = false;
		try {
			bi::named_mutex m(bi::open_only, named_mutex_name.c_str());
			already_running = true;
		} catch (const bi::interprocess_exception &) {}

		if ( !already_running ) {
			can_run = true;
			timer_thread = boost::thread(&impl::thread_func, this);
		} else {
			bi::named_mutex mutex(bi::open_only, named_mutex_name.c_str());
			bi::scoped_lock<bi::named_mutex> lock(mutex);
			bi::shared_memory_object shared_obj(bi::open_only, shared_object_name.c_str(), bi::read_only);
			bi::mapped_region region(shared_obj, bi::read_only);
			impl::timer_data *timer = new(region.get_address())impl::timer_data;
			assert(timer);

			can_run = timer->posix_time+(impl::sleep_time_sec*2) < static_cast<boost::uint32_t>(std::time(0));
			if ( can_run ) {
				bi::named_mutex::remove(named_mutex_name.c_str());
				bi::shared_memory_object::remove(shared_object_name.c_str());
				timer_thread = boost::thread(&impl::thread_func, this);
			}
		}
	}
	~impl() {
		thread_is_running = false;
		timer_thread.join();
		if ( can_run ) {
			bi::named_mutex::remove(named_mutex_name.c_str());
			bi::shared_memory_object::remove(shared_object_name.c_str());
		}
	}

	bool already_running() const { return !can_run; }
	int exec() const { return (can_run) ? main_function(argc, argv) : EXIT_FAILURE; }

private:
	static void thread_func(const void *data) {
		const single_application::impl *app = static_cast<const single_application::impl*>(data);

		bi::named_mutex mutex(bi::create_only, app->named_mutex_name.c_str());
		bi::shared_memory_object shared_obj(bi::create_only, app->shared_object_name.c_str(), bi::read_write);
		shared_obj.truncate(sizeof(timer_data));
		bi::mapped_region region(shared_obj, bi::read_write);
		while ( app->thread_is_running ) {
			{
				bi::scoped_lock<bi::named_mutex> lock(mutex);
				(new(region.get_address())timer_data)->posix_time = static_cast<boost::uint32_t>(std::time(0));
			}
			boost::this_thread::sleep(boost::posix_time::seconds(sleep_time_sec));
		}
	}

	enum { sleep_time_sec = 1 };
	struct timer_data {
		boost::uint32_t posix_time;
	};

	main_function_ptr main_function;
	int argc;
	char **argv;

	std::string named_mutex_name;
	std::string shared_object_name;

	bool can_run;
	volatile bool thread_is_running;
	boost::thread timer_thread;
};

/**************************************************************************/

single_application::single_application(single_application::main_function_ptr main, int argc, char **argv)
	:pimpl(new impl(main, argc, argv))
{}

single_application::~single_application()
{ delete pimpl; }

bool single_application::already_running() const { return pimpl->already_running(); }

int single_application::exec() const { return pimpl->exec(); }

/**************************************************************************/

} // ns single_application
