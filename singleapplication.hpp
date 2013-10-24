#ifndef _singleapplication_hpp
#define _singleapplication_hpp

#include <string>
#include <ctime>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

/***************************************************************************/

struct single_application: private boost::noncopyable {
	typedef int(*main_function_type)(int, char **);

	single_application(int argc, char **argv, main_function_type main)
		:argc(argc)
		,argv(argv)
		,maybe_run(false)
		,thread_is_running(true)
		,main_function(main)
	{
		prepare_unique_names(argv[0], named_mutex_name, shared_object_name);
		bool already_running = false;
		try {
			boost::interprocess::named_mutex m(boost::interprocess::open_only, named_mutex_name.c_str());
			already_running = true;
		} catch (const boost::interprocess::interprocess_exception& e) {
			maybe_run = true;
		}

		if ( !already_running ) {
			timer_thread = boost::thread(&single_application::thread_func, this);
		} else {
			boost::interprocess::named_mutex mutex(
				 boost::interprocess::open_only
				,named_mutex_name.c_str()
			);
			boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
			boost::interprocess::shared_memory_object shared_obj(
				 boost::interprocess::open_only
				,shared_object_name.c_str()
				,boost::interprocess::read_only
			);
			boost::interprocess::mapped_region region(shared_obj, boost::interprocess::read_only);
			timer_data *timer = ((new(region.get_address())timer_data);
			std::assert(timer);
			maybe_run = timer->posix_time+(sleep_time_sec*2) < (boost::uint32_t)std::time(0));
			if ( !maybe_run ) {
				throw std::runtime_error("application already running");
			} else {
				boost::interprocess::named_mutex::remove(named_mutex_name.c_str());
				boost::interprocess::shared_memory_object::remove(shared_object_name.c_str());
				timer_thread = boost::thread(&single_application::thread_func, this);
			}
		}
	}
	~single_application() {
		thread_is_running = false;
		timer_thread.join();
		if ( maybe_run) {
			boost::interprocess::named_mutex::remove(named_mutex_name.c_str());
			boost::interprocess::shared_memory_object::remove(shared_object_name.c_str());
		}
	}

	int exec() const { return (maybe_run) ? main_function(argc, argv) : EXIT_FAILURE; }

private:
	static void prepare_unique_names(const std::string &arg0, std::string& nm, std::string& so) {
		static const char* named_mutex_unique_name = "_singleapplication_named_mutex_";
		static const char* shared_data_unique_name = "_singleapplication_shared_data_";
#ifdef WIN32
		const std::string::value_type separator[] = "\\\0";
#else
		const std::string::value_type separator[] = "/\0";
#endif
		std::string::const_iterator sep = std::find_end(arg0.begin(), arg0.end(), separator, separator+1);
		nm = ((sep == arg0.begin()) ? arg0 : std::string(sep+1, arg0.end())) + named_mutex_unique_name;
		so = ((sep == arg0.begin()) ? arg0 : std::string(sep+1, arg0.end())) + shared_data_unique_name;
	}

	void thread_func() {
		boost::interprocess::named_mutex mutex(boost::interprocess::open_only, named_mutex_name.c_str());
		boost::interprocess::shared_memory_object shared_obj(boost::interprocess::create_only, shared_object_name.c_str(), boost::interprocess::read_write);
		shared_obj.truncate(sizeof(timer_data));
		boost::interprocess::mapped_region region(shared_obj, boost::interprocess::read_write);
		while ( thread_is_running ) {
			{  boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
				(new(region.get_address())timer_data)->posix_time = std::time(0);
			}
			boost::this_thread::sleep(boost::posix_time::seconds(sleep_time_sec));
		}
	}

private:
	enum { sleep_time_sec = 1 };
	struct timer_data {
		boost::uint32_t posix_time;
	};

	int argc;
	char **argv;

	bool maybe_run;
	volatile bool thread_is_running;
	main_function_type main_function;

	std::string named_mutex_name;
	std::string shared_object_name;

	boost::thread timer_thread;
};

/***************************************************************************/

#endif // _singleapplication_hpp
