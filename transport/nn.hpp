#ifndef NN_HPP_INCLUDED
#define NN_HPP_INCLUDED

#include <nng/nng.h>
#include <nng/protocol/pair0/pair.h>

#include <cassert>
#include <cstring>
#include <algorithm>
#include <exception>

#if defined __GNUC__
#define nn_slow(x) __builtin_expect((x), 0)
#else
#define nn_slow(x) (x)
#endif

namespace nn
{

    class exception : public std::exception
    {
    public:
        exception(int e) : err(e) {}

        virtual const char *what() const throw()
        {
            return nng_strerror(err);
        }

        int num() const
        {
            return err;
        }

    private:
        int err;
    };

    inline int freemsg(void *msgm, uint64_t sz)
    {
        nng_free(msgm, sz);
        return 0;
    }

    class socket
    {
    public:
        inline socket()
        {
            int rc;
            if ((rc = nng_pair0_open(&s)) != 0)
            {
                throw nn::exception(rc);
            }
        }

        inline ~socket()
        {
            int rc = nng_close(s);
            assert(rc == 0);
        }

        inline int bind(const char *addr)
        {
            int rc;
            if ((rc = nng_listen(s, addr, NULL, 0)) != 0)
            {
                throw nn::exception(rc);
            }
            return rc;
        }

        inline int connect(const char *addr)
        {
            int rc;
            if ((rc = nng_dial(s, addr, NULL, NNG_FLAG_NONBLOCK)) != 0)
            {
                throw nn::exception(rc);
            }
            return rc;
        }

        inline int send(void *buf, size_t len, int flags)
        {
            int rc = nng_send(s, buf, len, flags);
            if (nn_slow(rc != 0))
            {
                if (nn_slow(rc != NNG_EAGAIN))
                    throw nn::exception(rc);
                return -1;
            }
            return rc;
        }

        inline int recv(void **buf, int flags)
        {
            size_t len;
            int rc = nng_recv(s, buf, &len, flags);
            if (nn_slow(rc != 0))
            {
                if (nn_slow(rc != NNG_EAGAIN))
                {
                    throw nn::exception(rc);
                }
                return -1;
            }
            fflush(stdout);
            return len;
        }

        inline void setsockopt_ms(const char *option, int optval)
        {
            int rc;
            if ((rc = nng_setopt_ms(s, option, optval)) != 0)
            {
                throw nn::exception(rc);
            }
        }

        nng_socket s;

    private:
        /*  Prevent making copies of the socket by accident. */
        socket(const socket &);
        void operator=(const socket &);
    };

} // namespace nn

#undef nn_slow

#endif