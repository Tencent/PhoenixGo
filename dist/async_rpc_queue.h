/*
 * Tencent is pleased to support the open source community by making PhoenixGo available.
 * 
 * Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
 * 
 * Licensed under the BSD 3-Clause License (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     https://opensource.org/licenses/BSD-3-Clause
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <atomic>
#include <chrono>

#include <grpc++/grpc++.h>

template<class Req, class Resp>
using AsyncRpcFunc = std::function<std::unique_ptr<grpc::ClientAsyncResponseReader<Resp>>
                                   (grpc::ClientContext*, const Req&, grpc::CompletionQueue*)>;

#define BindAsyncRpcFunc(stub, func) \
    std::bind(&std::remove_reference<decltype(stub)>::type::func, stub, \
              std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)

template<class Resp>
using AsyncRpcCallback = std::function<void(grpc::Status&, Resp&)>;

struct AsyncClientCallBase
{
    virtual ~AsyncClientCallBase() {}
    virtual void Complete() = 0;
};

template<class Resp>
struct AsyncClientCall: public AsyncClientCallBase
{
    Resp resp;
    grpc::ClientContext context;
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncResponseReader<Resp>> response_reader;
    AsyncRpcCallback<Resp> callback;

    void Complete() override { callback(status, resp); }
};

class AsyncRpcQueue
{

 public:
    AsyncRpcQueue()
        : m_size(0), m_is_shutdown(false)
    {
    }

    template<class Req, class Resp>
    void Call(AsyncRpcFunc<Req, Resp> fn, const Req &req, AsyncRpcCallback<Resp> callback, int timeout_ms = -1)
    {
        auto call = new AsyncClientCall<Resp>;
        call->callback = callback;
        if (timeout_ms > 0) {
            call->context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms));
        }
        call->response_reader = fn(&call->context, req, &m_cq);
        call->response_reader->Finish(&call->resp, &call->status, (void*)call);

        ++m_size;
    }

    void Complete(int n = -1)
    {
        void *got_tag;
        bool ok = false;
        for (int i = 0; (n < 0 || i < n) && !m_is_shutdown && m_cq.Next(&got_tag, &ok); ++i) {
            std::unique_ptr<AsyncClientCallBase> call(static_cast<AsyncClientCallBase*>(got_tag));
            --m_size;
            call->Complete();
        }
    }

    void Shutdown()
    {
        m_is_shutdown = true;
        m_cq.Shutdown();
    }

    int Size()
    {
        return m_size;
    }

 private:
    grpc::CompletionQueue m_cq;
    std::atomic<int> m_size;
    std::atomic<bool> m_is_shutdown;
};
