//-
// Copyright 2026 Autodesk, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//+

#ifndef BIFUSD_TBBINITGUARD_H
#define BIFUSD_TBBINITGUARD_H

#if USD_TBB_VERSION_MAJOR < 2021
// Required for tbb::task_scheduler_init::blocking_terminate().
#define TBB_PREVIEW_WAITING_FOR_WORKERS 1
#include <tbb/task_scheduler_init.h>
#else
#include <oneapi/tbb/global_control.h>
#endif

#include <iostream>

namespace Bifusd {

//==============================================================================
// CLASS TBBInitGuard
//==============================================================================

/// \brief Properly initialize and terminate TBB's scheduler
///
/// Ensures that the TBB workers threads have exited before allowing the
/// process to exit. Without this, it is possible that the process exits before
/// the worker threads have been terminated. If this happens, Valgrind and the
/// Leak Sanitizer will report this associated memory leaks. This makes hard to
/// reliably use these tools.
class TBBInitGuard {
public:
    enum Policy : unsigned char { strict, relaxed };

    /*----- member functions -----*/

    /// \brief Constructor
    ///
    /// We must ensure that the TBB workers threads have exited before allowing
    /// the process to exit. Without this, it is possible that the process exits
    /// before the worker threads have been terminated. If this happens,
    /// Valgrind and the Leak Sanitizer will report this associated memory
    /// leaks. This makes hard to reliably use these tools.
    ///
    /// The following line of code resolves these issues.
    ///
    /// See: What's New? Intel® Threading Building Blocks 4.2
    /// https://software.intel.com/en-us/articles/
    ///     whats-new-intel-threading-building-blocks-42
    ///
    /// """The new community preview feature allows waiting until all worker
    /// threads terminate. This may be needed if an application forks processes,
    /// or if the Intel TBB dynamic library can be unloaded at runtime (e.g. if
    /// Intel TBB is a part of a plugin). To enable waiting for workers,
    /// initialize the task_scheduler_init object this way:"""
    ///
    /// See also: TBB initialization, termination, and resource management
    ///           details, juicy and gory.
    /// https://software.intel.com/en-us/blogs/2011/04/09/tbb-initialization-termination-and-resource-management-details-juicy-and-gory
    explicit TBBInitGuard(Policy policy = Policy::relaxed) : m_policy{policy} {}
    ~TBBInitGuard() {
        if (!finalize() && m_policy == strict) {
            // This should never happen!
            std::cerr << "TBB finalization error: "
                         "Failed to finalize the TBB scheduler!"
                      << std::endl;
            std::terminate();
        }
    }

private:
#if USD_TBB_VERSION_MAJOR < 2021
    bool finalize() { return m_handle.blocking_terminate(std::nothrow); }
    tbb::task_scheduler_init m_handle{};
#else
    bool finalize() { return oneapi::tbb::finalize(m_handle, std::nothrow); }
    oneapi::tbb::task_scheduler_handle m_handle{oneapi::tbb::attach{}};
#endif
    Policy m_policy;
};
} // namespace Bifusd

#endif // BIFUSD_TBBINITGUARD_H
