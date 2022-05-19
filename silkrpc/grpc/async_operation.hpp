/*
    Copyright 2020 The Silkrpc Authors

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef SILKRPC_GRPC_ASYNC_OPERATION_HPP_
#define SILKRPC_GRPC_ASYNC_OPERATION_HPP_

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_tracking.hpp>
#include <boost/asio/detail/handler_work.hpp>
#include <boost/asio/detail/memory.hpp>

namespace silkrpc {

// Base class for gRPC async operations using Asio completion tokens.
template<typename R, typename... Args>
class async_operation BOOST_ASIO_INHERIT_TRACKED_HANDLER {
public:
    typedef async_operation operation_type;

    void complete(void* owner, Args... args) {
        func_(owner, this, args...);
    }

protected:
    typedef R (*func_type)(void*, async_operation*, Args...);

    async_operation(func_type func) : func_(func) {}

    // Prevents deletion through this type.
    ~async_operation() {}

private:
    func_type func_;
};

template <typename Handler, typename IoExecutor, typename Reply>
class async_reply_operation : public async_operation<void, boost::system::error_code, Reply> {
public:
    BOOST_ASIO_DEFINE_HANDLER_PTR(async_reply_operation);

    async_reply_operation(Handler& h, const IoExecutor& io_ex)
    : async_operation<void, boost::system::error_code, Reply>(&async_reply_operation::do_complete), handler_(BOOST_ASIO_MOVE_CAST(Handler)(h)), work_(handler_, io_ex)
    {}

    static void do_complete(void* owner, async_operation<void, boost::system::error_code, Reply>* base, boost::system::error_code error = {}, Reply reply = {}) {
        // Take ownership of the handler object.
        async_reply_operation* h{static_cast<async_reply_operation*>(base)};
        ptr p = {boost::asio::detail::addressof(h->handler_), h, h};

        BOOST_ASIO_HANDLER_COMPLETION((*h));

        // Take ownership of the operation's outstanding work.
        boost::asio::detail::handler_work<Handler, IoExecutor> w(
            BOOST_ASIO_MOVE_CAST2(boost::asio::detail::handler_work<Handler, IoExecutor>)(h->work_));

        // Make a copy of the handler so that the memory can be deallocated before
        // the upcall is made. Even if we're not about to make an upcall, a
        // sub-object of the handler may be the true owner of the memory associated
        // with the handler. Consequently, a local copy of the handler is required
        // to ensure that any owning sub-object remains valid until after we have
        // deallocated the memory here.

        boost::asio::detail::move_binder2<Handler, boost::system::error_code, Reply> handler{
            0, BOOST_ASIO_MOVE_CAST(Handler)(h->handler_), error, BOOST_ASIO_MOVE_CAST(Reply)(reply)};
        p.h = boost::asio::detail::addressof(handler.handler_);
        p.reset();

        // Make the upcall if required.
        if (owner) {
            boost::asio::detail::fenced_block b(boost::asio::detail::fenced_block::half);
            BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
            w.complete(handler, handler.handler_);
            BOOST_ASIO_HANDLER_INVOCATION_END;
        }
    }

private:
    Handler handler_;
    boost::asio::detail::handler_work<Handler, IoExecutor> work_;
};

template <typename Handler, typename IoExecutor>
class async_reply_operation<Handler, IoExecutor, void> : public async_operation<void, boost::system::error_code> {
public:
    BOOST_ASIO_DEFINE_HANDLER_PTR(async_reply_operation);

    async_reply_operation(Handler& h, const IoExecutor& io_ex)
    : async_operation<void, boost::system::error_code>(&async_reply_operation::do_complete), handler_(BOOST_ASIO_MOVE_CAST(Handler)(h)), work_(handler_, io_ex)
    {}

    static void do_complete(void* owner, async_operation<void, boost::system::error_code>* base, boost::system::error_code error = {}) {
        // Take ownership of the handler object.
        async_reply_operation* h{static_cast<async_reply_operation*>(base)};
        ptr p = {boost::asio::detail::addressof(h->handler_), h, h};

        BOOST_ASIO_HANDLER_COMPLETION((*h));

        // Take ownership of the operation's outstanding work.
        boost::asio::detail::handler_work<Handler, IoExecutor> work(
            BOOST_ASIO_MOVE_CAST2(boost::asio::detail::handler_work<Handler, IoExecutor>)(h->work_));

        // Make a copy of the handler so that the memory can be deallocated before
        // the upcall is made. Even if we're not about to make an upcall, a
        // sub-object of the handler may be the true owner of the memory associated
        // with the handler. Consequently, a local copy of the handler is required
        // to ensure that any owning sub-object remains valid until after we have
        // deallocated the memory here.
        boost::asio::detail::binder1<Handler, boost::system::error_code> handler{h->handler_, error};
        p.h = boost::asio::detail::addressof(handler.handler_);
        p.reset();

        // Make the upcall if required.
        if (owner) {
            boost::asio::detail::fenced_block b(boost::asio::detail::fenced_block::half);
            BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_));
            work.complete(handler, handler.handler_);
            BOOST_ASIO_HANDLER_INVOCATION_END;
        }
    }

private:
    Handler handler_;
    boost::asio::detail::handler_work<Handler, IoExecutor> work_;
};

template <typename Handler, typename IoExecutor>
using async_noreply_operation = async_reply_operation<Handler, IoExecutor, void>;

} // namespace silkrpc

#endif // SILKRPC_GRPC_ASYNC_OPERATION_HPP_
