// ntp_client.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once
#ifndef __MACHO_NTPCLIENT_HPP
#define __MACHO_NTPCLIENT_HPP

#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/posix_time.hpp> // Need I/O.
#include "..\config\config.hpp"
#include "exception_base.hpp"
#include "stringutils.hpp"

namespace macho{

class ntp_client{
public:
    struct exception : virtual public exception_base {};
private :
    #define BOOST_THROW_NTP_CLIENT_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( exception, no, message )
public:

    static bool get_time( boost::posix_time::ptime &time ){
        return get_time( "pool.ntp.org", time );
    }

    static bool get_time( std::wstring ntp_server, boost::posix_time::ptime &time ){
        std::string ansi_ntp_server = stringutils::convert_unicode_to_ansi(ntp_server);
        return get_time( ansi_ntp_server, time );
    }

    static bool get_time( std::string ntp_server, boost::posix_time::ptime &time ){
        try{
            time = get_time( ntp_server );
        }
        catch(...){
            return false;
        }
        return true;
    }

    static boost::posix_time::ptime get_time(){
        return get_time("pool.ntp.org");
    }

    static boost::posix_time::ptime get_time( std::wstring ntp_server ){
        std::string ansi_ntp_server = stringutils::convert_unicode_to_ansi(ntp_server);
        return get_time( ansi_ntp_server );
    }

    static boost::posix_time::ptime get_time( std::string ntp_server ){

        using namespace boost::posix_time;
        using boost::asio::ip::udp;
        boost::asio::io_service io_service;
        udp::resolver resolver(io_service);
        udp::resolver::query query(udp::v4(), ntp_server, "ntp");
        udp::endpoint receiver_endpoint = *resolver.resolve(query);

        udp::endpoint sender_endpoint;

        boost::uint8_t data[48] = {
        0x1B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        };

        udp::socket socket(io_service);
        socket.open(udp::v4());
        ntp_client ntpclient( io_service, socket );
        boost::system::error_code ec;
        socket.send_to( boost::asio::buffer(data), receiver_endpoint ) ;
        ntpclient.receive( boost::asio::buffer(data), boost::posix_time::seconds(10), ec ) ;

        if( ec ){
#if _UNICODE
            BOOST_THROW_NTP_CLIENT_EXCEPTION( ERROR_TIMEOUT, boost::str(boost::wformat(_T("Can't get date time information from ntp server (%s)") ) % stringutils::convert_ansi_to_unicode(ntp_server)));
#else
            BOOST_THROW_NTP_CLIENT_EXCEPTION( ERROR_TIMEOUT, boost::str( boost::format( _T("Can't get date time information from ntp server (%s)") )% ntp_server ) );
#endif
        }

        typedef boost::uint32_t u32;
        const u32 iPart(
            static_cast<u32>(data[40]) << 24
            | static_cast<u32>(data[41]) << 16
            | static_cast<u32>(data[42]) << 8
            | static_cast<u32>(data[43])
            );
        const u32 fPart(
            static_cast<u32>(data[44]) << 24
            | static_cast<u32>(data[45]) << 16
            | static_cast<u32>(data[46]) << 8
            | static_cast<u32>(data[47])
            );
        const ptime pt(
            boost::gregorian::date(1900,1,1),
            milliseconds(
                (int64_t)(iPart * 1.0E3)
                + (int64_t)(fPart * 1.0E3 / 0x100000000ULL))
            );
        return pt;
    }
private:
    ntp_client( boost::asio::io_service& io_service, boost::asio::ip::udp::socket& socket ) 
        : _socket(socket)
        , _io_service(io_service)
        , _deadline(_io_service) {
        // No deadline is required until the first socket operation is started. We
        // set the deadline to positive infinity so that the actor takes no action
        // until a specific deadline is set.
        _deadline.expires_at(boost::posix_time::pos_infin);

        // Start the persistent actor that checks for deadline expiry.
        check_deadline();   
    }

    std::size_t receive(const boost::asio::mutable_buffer& buffer,
        boost::posix_time::time_duration timeout, boost::system::error_code& ec){

        // Set a deadline for the asynchronous operation.
        _deadline.expires_from_now(timeout);

        // Set up the variables that receive the result of the asynchronous
        // operation. The error code is set to would_block to signal that the
        // operation is incomplete. Asio guarantees that its asynchronous
        // operations will never fail with would_block, so any other value in
        // ec indicates completion.
        ec = boost::asio::error::would_block;
        std::size_t length = 0;

        // Start the asynchronous operation itself. The handle_receive function
        // used as a callback will update the ec and length variables.
        _socket.async_receive(boost::asio::buffer(buffer),
            boost::bind(&ntp_client::handle_receive, _1, _2, &ec, &length));

        // Block until the asynchronous operation has completed.
        do _io_service.run_one(); while (ec == boost::asio::error::would_block);

        return length;
    }

    void check_deadline(){
        // Check whether the deadline has passed. We compare the deadline against
        // the current time since a new asynchronous operation may have moved the
        // deadline before this actor had a chance to run.
        if (_deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now())
        {
            // The deadline has passed. The outstanding asynchronous operation needs
            // to be cancelled so that the blocked receive() function will return.
            //
            // Please note that cancel() has portability issues on some versions of
            // Microsoft Windows, and it may be necessary to use close() instead.
            // Consult the documentation for cancel() for further information.
            _socket.cancel();

            // There is no longer an active deadline. The expiry is set to positive
            // infinity so that the actor takes no action until a new deadline is set.
            _deadline.expires_at(boost::posix_time::pos_infin);
        }

        // Put the actor back to sleep.
        _deadline.async_wait(boost::bind(&ntp_client::check_deadline, this));
    }

    static void handle_receive(
        const boost::system::error_code& ec, std::size_t length,
        boost::system::error_code* out_ec, std::size_t* out_length){
        *out_ec = ec;
        *out_length = length;
    }
    boost::asio::io_service&       _io_service;
    boost::asio::ip::udp::socket&  _socket;
    boost::asio::deadline_timer    _deadline;
};

};

#endif