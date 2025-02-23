/*
 * LockedDebugStream.hpp
 *
 *  Created on: Mar 5, 2024
 *      Author: martin.oberzalek
 */
#pragma once

#include <os.hpp>
#include <wlib.hpp>
#include <bslib.hpp>
#include <memory>

class LockedDebugStream
{
public:
	class WrappedHandle
	{
	private:
		os::mutex * m_lock;
		wlib::StringSink_Interface * sink;

	public:
		WrappedHandle( os::mutex & m_lock_, wlib::StringSink_Interface & sink_ )
		: m_lock( &m_lock_ ),
		  sink( &sink_ )
		{
			m_lock->lock();
		}

		WrappedHandle( WrappedHandle && other )
		: m_lock( other.m_lock ),
		  sink( other.sink )
		{
			other.reset();
		}

		WrappedHandle( const WrappedHandle & other ) = delete;

		WrappedHandle & operator=( const WrappedHandle & other ) = delete;
        
		WrappedHandle & operator=( WrappedHandle && other )
		{
			reset();
			m_lock = other.m_lock;
			sink = other.sink;

			other.release();

			return *this;
		}

		WrappedHandle() = delete;
		~WrappedHandle() {
			reset();
		}

		void write( const char *data ) { (*sink)(data); }
		void write( const std::string_view & data ) { (*sink)(data.data(),data.size()); }
		void write( const char *data, uint32_t len ) { (*sink)(data,len); }

		void reset() {
			if( m_lock ) {
				m_lock->unlock();
			}
			release();
		}



	private:
		void release() {
			m_lock = nullptr;
			sink = nullptr;
		}

	};

	class LockedStringSinkInterface : public wlib::StringSink_Interface
	{
	public:
		bool operator()( const char *data ) override {
			LockedDebugStream::instance().get().write(data);
            return true;
		}
		bool operator()( const char *data, uint32_t len ) override {
			LockedDebugStream::instance().get().write(data, len );
            return true;
		}
	};

protected:
	os::mutex m_lock;
public:

	static LockedDebugStream & instance();

	WrappedHandle get();
};

