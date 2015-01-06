/*
 * Copyright 2014 Marios Kogias <marioskogias@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef ZTRACER_H

#define ZTRACER_H

#include <sstream>

extern "C" {
#include <zipkin_c.h>
}

namespace ZTracer {
	using std::string;

	static inline int ztrace_init() { return blkin_init(); }

	class Endpoint : private blkin_endpoint {
		private:
			string _ip; // storage for blkin_endpoint.ip, see copy_ip()
			string _name; // storage for blkin_endpoint.name, see copy_name()

			friend class Trace;
		public:
			Endpoint(const char *ip, int port, const char *name)
			{
				blkin_init_endpoint(this, ip, port, name);
			}

      // Endpoint assumes that ip and name will be string literals, and
      // avoids making a copy and freeing on destruction.  if you need to
      // initialize Endpoint with a temporary string, copy_ip/copy_name()
      // will store it in a std::string and assign from that
      void copy_ip(const string &newip)
      {
        _ip.assign(newip);
        ip = _ip.c_str();
      }
      void copy_name(const string &newname)
      {
        _name.assign(newname);
        name = _name.c_str();
      }

      void set_port(int p) { port = p; }
	};

	class Trace : private blkin_trace {
    private:
      string _name; // storage for blkin_trace.name, see copy_name()

		public:
      // default constructor zero-initializes blkin_trace
      // valid() will return false on a default-constructed Trace until init()
      Trace()
      {
        // zero-initialize so valid() returns false
        name = NULL;
        info.trace_id = 0;
        info.span_id = 0;
        info.parent_span_id = 0;
        endpoint = NULL;
      }

      // construct a Trace with an optional parent
			Trace(const char *name, const Endpoint *ep, const Trace *parent = NULL)
      {
        if (parent && parent->valid()) {
          blkin_init_child(this, parent, ep ? : parent->endpoint, name);
        } else {
				  blkin_init_new_trace(this, name, ep);
        }
      }

      // construct a Trace from blkin_trace_info
			Trace(const char *name, const Endpoint *ep,
            const blkin_trace_info *i, bool child=false)
			{
				if (child)
					blkin_init_child_info(this, i, ep, name);
				else {
					blkin_init_new_trace(this, name, ep);
					set_info(i);
				}
			}

      Trace(const Trace &rhs) : _name(rhs._name)
      {
        if (rhs.name == rhs._name.c_str())
          name = _name.c_str();
        else
          name = rhs.name;
        info = rhs.info;
        endpoint = rhs.endpoint;
      }

      // return true if the Trace has been initialized
      bool valid() const { return info.trace_id != 0; }
      operator bool() const { return valid(); }

      // (re)initialize a Trace with an optional parent
			int init(const char *name, const Endpoint *ep, const Trace *parent = NULL)
      {
        if (parent && parent->valid())
          return blkin_init_child(this, parent, ep ? : parent->endpoint, name);

        return blkin_init_new_trace(this, name, ep);
      }

      // (re)initialize a Trace from blkin_trace_info
			int init(const char *name, const Endpoint *ep,
               const blkin_trace_info *i, bool child=false)
      {
				if (child)
					return blkin_init_child_info(this, i, ep, _name.c_str());

        int r = blkin_init_new_trace(this, _name.c_str(), ep);
        if (r == 0)
          set_info(i);
        return r;
      }

      // Trace assumes that name will be a string literal, and avoids
      // making a copy and freeing on destruction.  if you need to
      // initialize Trace with a temporary string, copy_name() will store
      // it in a std::string and assign from that
      void copy_name(const string &newname)
      {
        _name.assign(newname);
        name = _name.c_str();
      }

      const blkin_trace_info* get_info() const { return &info; }
      void set_info(const blkin_trace_info *i) { info = *i; }

      // record key-value annotations
			void keyval(const char *key, const char *val)
			{
        if (valid())
          BLKIN_KEYVAL_STRING(this, endpoint, key, val);
			}
			void keyval(const char *key, int64_t val)
			{
        if (valid())
          BLKIN_KEYVAL_INTEGER(this, endpoint, key, val);
			}
			void keyval(const char *key, const char *val, const Endpoint *ep)
			{
        if (valid())
          BLKIN_KEYVAL_STRING(this, ep, key, val);
			}
			void keyval(const char *key, int64_t val, const Endpoint *ep)
			{
        if (valid())
          BLKIN_KEYVAL_INTEGER(this, ep, key, val);
			}

      // record timestamp annotations
			void event(const char *event)
			{
        if (valid())
          BLKIN_TIMESTAMP(this, endpoint, event);
			}
			void event(const char *event, const Endpoint *ep)
			{
        if (valid())
          BLKIN_TIMESTAMP(this, ep, event);
			}
	};

}
#endif /* end of include guard: ZTRACER_H */