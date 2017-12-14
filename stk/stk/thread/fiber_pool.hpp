
#ifndef STK_THREAD_FIBER_POOL_HPP
#define STK_THREAD_FIBER_POOL_HPP
#pragma once

#include <stk/thread/function_wrapper.hpp>
#include <stk/thread/barrier.hpp>
#include <stk/container/locked_queue.hpp>
#include <stk/thread/atomic_spin_lock.hpp>
#include <stk/thread/boost_thread_kernel.hpp>
#include <boost/noncopyable.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/fiber/all.hpp>
#include <cstdint>

#ifndef STK_NO_FIBER_POOL_BIND_TO_PROCESSOR
#include <stk/thread/bind/bind_processor.hpp>
#endif

namespace stk { namespace thread {

template <typename Allocator = boost::fibers::fixedsize_stack, typename QTraits = locked_queue_traits, typename ThreadTraits = boost_thread_traits>
class fiber_pool : boost::noncopyable
{
    using thread_traits = ThreadTraits;
	using queue_traits = QTraits;
    using allocator_type = Allocator;

    using thread_type = typename thread_traits::thread_type;
    using fiber_type = boost::fibers::fiber;

    template <typename T>
    using packaged_task = boost::fibers::packaged_task<T>;

    using mutex_type = typename thread_traits::mutex_type;
	using fiber_mutex_type = boost::fibers::mutex;

    using queue_type = typename queue_traits::template queue_type<function_wrapper, std::allocator<function_wrapper>, fiber_mutex_type>;

    void os_thread(std::size_t nThreads, std::size_t nFibersPerThread, unsigned int idx)
    {
#ifndef STK_NO_FIBER_POOL_BIND_TO_PROCESSOR
        bind_to_processor(idx);
#endif
        //boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>(/*nThreads*/);
        m_barrier.wait();

		auto start = idx * nFibersPerThread;
		auto end = start + nFibersPerThread;
		for (std::size_t i = start; i < end; ++i)
			m_fibers[i] = boost::fibers::fiber(boost::fibers::launch::dispatch, std::allocator_arg, m_alloc, [this]() { worker_fiber(); });
		
		lock_type lk{ m_mtx };
		m_shutdownCondition.wait(lk, [this]() { return m_done.load(); });
        GEOMETRIX_ASSERT(m_done);

		for (std::size_t i = start; i < end; ++i)
		{
			try
			{
				if (m_fibers[i].joinable())
					m_fibers[i].join();
			} 
			catch(...)
			{}
		}
    }

    void worker_fiber()
    {
		function_wrapper task;
		while (!m_done.load())
		{
			if (queue_traits::try_pop(m_tasks, task))
				task();
			boost::this_fiber::yield();
		}
    }
	
public:

    template <typename T>
    using future = boost::fibers::future<T>;

    fiber_pool(std::size_t nFibersPerThread, const allocator_type& alloc, unsigned int nOSThreads = std::thread::hardware_concurrency() - 1)
        : m_done(false)
        , m_alloc(alloc)
		, m_barrier(nOSThreads + 1)
    {
        GEOMETRIX_ASSERT(nOSThreads > 1);//1 thread? why?
		if (nOSThreads < 2)
			throw std::invalid_argument("fiber pool should have at least 2 OS threads.");
		
        try
        {
            auto nCPUS = std::thread::hardware_concurrency();
            m_threads.reserve(nOSThreads);
			m_fibers.resize(nFibersPerThread * nOSThreads);

            for(unsigned int i = 0; i < nOSThreads; ++i)
                m_threads.emplace_back([this](std::size_t nThreads, std::size_t nFibersPerThread, unsigned int idx){ os_thread(nThreads, nFibersPerThread, idx); }, nOSThreads, nFibersPerThread, i % nCPUS);
            m_barrier.wait();
        }
        catch(...)
        {
            shutdown();
            throw;
        }
    }

    ~fiber_pool()
    {
        shutdown();
    }

    template <typename Action>
    future<decltype(std::declval<Action>()())> send(const Action& x)
    {
        return send_impl(static_cast<const Action&>(x));
    }

    template <typename Action>
    future<decltype(std::declval<Action>()())> send(Action&& x)
    {
        return send_impl(std::forward<Action>(x));
    }

private:

    template <typename Action>
    future<decltype(std::declval<Action>()())> send_impl(Action&& m)
    {
        using result_type = decltype(m());
        packaged_task<result_type()> task(std::forward<Action>(m));
        auto result = task.get_future();

        queue_traits::push(m_tasks, function_wrapper(std::move(task)));

        return std::move(result);
    } 

    void shutdown()
    {
        bool b = false;
        if( m_done.compare_exchange_strong(b, true) )
        {
            m_shutdownCondition.notify_all();

            boost::for_each(m_threads, [](thread_type& t)
            {
                try
                {
                    if (t.joinable())
                        t.join();
                }
                catch (...)
                {}
            });
        }
    }

    std::vector<thread_type>                m_threads;
    std::vector<fiber_type>                 m_fibers;
    std::atomic<bool>                       m_done;
    queue_type                              m_tasks;

    using lock_type = std::unique_lock<mutex_type>;
    mutex_type                              m_mtx;
    boost::fibers::condition_variable_any   m_shutdownCondition;
    allocator_type                          m_alloc;
	barrier<mutex_type>						m_barrier;
};

}}//! namespace stk::thread;

#endif // STK_THREAD_FIBER_POOL_HPP
