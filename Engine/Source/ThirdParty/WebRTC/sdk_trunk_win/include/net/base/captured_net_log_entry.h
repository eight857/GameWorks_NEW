// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_CAPTURED_NET_LOG_ENTRY_H_
#define NET_BASE_CAPTURED_NET_LOG_ENTRY_H_

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "net/base/net_log.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace net {

// CapturedNetLogEntry is much like NetLog::Entry, except it has its own copy of
// all log data, so a list of entries can be gathered over the course of a test,
// and then inspected at the end.  It is intended for testing only, and is part
// of the net_test_support project.
struct CapturedNetLogEntry {
  // Ordered set of logged entries.
  typedef std::vector<CapturedNetLogEntry> List;

  CapturedNetLogEntry(NetLog::EventType type,
                      const base::TimeTicks& time,
                      NetLog::Source source,
                      NetLog::EventPhase phase,
                      scoped_ptr<base::DictionaryValue> params);
  // Copy constructor needed to store in a std::vector because of the
  // scoped_ptr.
  CapturedNetLogEntry(const CapturedNetLogEntry& entry);

  ~CapturedNetLogEntry();

  // Equality operator needed to store in a std::vector because of the
  // scoped_ptr.
  CapturedNetLogEntry& operator=(const CapturedNetLogEntry& entry);

  // Attempt to retrieve an value of the specified type with the given name
  // from |params|.  Returns true on success, false on failure.  Does not
  // modify |value| on failure.
  bool GetStringValue(const std::string& name, std::string* value) const;
  bool GetIntegerValue(const std::string& name, int* value) const;
  bool GetListValue(const std::string& name, base::ListValue** value) const;

  // Same as GetIntegerValue, but returns the error code associated with a
  // log entry.
  bool GetNetErrorCode(int* value) const;

  // Returns the parameters as a JSON string, or empty string if there are no
  // parameters.
  std::string GetParamsJson() const;

  NetLog::EventType type;
  base::TimeTicks time;
  NetLog::Source source;
  NetLog::EventPhase phase;
  scoped_ptr<base::DictionaryValue> params;
};

}  // namespace net

#endif  // NET_BASE_CAPTURED_NET_LOG_ENTRY_H_
