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

#include "include/GraphExecutorFactory.h"

#include "include/GraphExecutor.h"
#include "include/GraphExecutorConstants.h"
#include "include/GraphExecutorFactoryInternal.h"
#include "include/StreamObserver.h"

// Amino
#include <Amino/Core/String.h>
#include <Amino/Core/TaskObserver.h>
#include <Amino/Executor/Executable.h>
#include <Amino/Library/ConstLibrary.h>

// Bifrost
#include <BifrostGraph/Executor/Utility.h> // ConfigEnv

// C++ Standard Library
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <shared_mutex>
#include <utility>

using ExecutableRegistry = std::map<Amino::String, Amino::Executable>;

namespace {
using BifrostUsd::GraphExecutor::StreamObserver;
using BifrostUsd::GraphExecutor::VerbosityLevel;

ExecutableRegistry& getExecutableRegistry() {
    static ExecutableRegistry registry;
    return registry;
}

// Add a mutex to protect access to the registry
std::mutex& getRegistryMutex() {
    static std::mutex registryMutex;
    return registryMutex;
}

Amino::ConstLibrary g_libInstance;

// Use a shared_mutex to protect access to the Library.
// Multiple concurrent threads can read from the Library and create Executables.
// When the library is initialized (or reloaded), an exclusive access is
// required to make necessary changes to the Library and ExecutableRegistry.
std::shared_mutex& getLibraryMutex() {
    static std::shared_mutex libraryMutex;
    return libraryMutex;
}

// Access the Library.
// This function assumes the Library is already initialized.
// Caller should acquire a SHARED lock before calling this function.
Amino::ConstLibrary& getLibrary() { return g_libInstance; }

// Initializes the library if not already done.
// Call this function before accessing the Library through getLibrary().
// Note: This function acquires an EXCLUSIVE lock internally.
void ensureLibrary() {
    // ConstLibrary::isValid() is not thread-safe (not using std::atomic),
    // so we need to acquire an EXCLUSIVE lock to check and initialize the
    // library if needed.
    std::unique_lock<std::shared_mutex> lock(getLibraryMutex());
    if (!g_libInstance.isValid()) {
        // Use the ConstLibraryBuilder to load config files and build the
        // ConstLibrary singleton.
        Amino::ConstLibraryBuilder builder;
        std::cout << BifrostUsd::GraphExecutor::kCtxGExecFact
                  << "Initializing Bifrost Library.\n";

        // Load all the Bifrost config files.
        auto configEnv = BifrostGraph::Executor::makeOwner<
            BifrostGraph::Executor::Utility::ConfigEnv>();
        assert(configEnv->isValid());
        auto observer = Amino::TaskObserver::make<StreamObserver<std::ostream>>(
            std::cout, std::cerr);
        observer->setVerbosityLevel(VerbosityLevel::eErrorsAndWarnings);
        observer->setPrintPrefix(
            BifrostUsd::GraphExecutor::kCtxGExecFactLoadConfigFile);
        for (const auto& pathname :
             configEnv->values("bifrost_pack_config_files")) {
            std::cout << BifrostUsd::GraphExecutor::kCtxGExecFact
                      << "Loading Bifrost Pack Config file: "
                      << pathname.c_str() << "\n";
            builder.loadConfigFile(observer, pathname);
        }

        // Build the ConstLibrary singleton.
        g_libInstance = builder.build();
        assert(g_libInstance.isValid());
    }
}

Amino::Executable makeExecutable_impl(Amino::StringView compound_name,
                                      StringArray*      errors) {
    auto observer =
        errors ? Amino::TaskObserver::make<StreamObserver<std::ostream>>(
                     std::cout, std::cerr, *errors)
               : Amino::TaskObserver::make<StreamObserver<std::ostream>>(
                     std::cout, std::cerr);
    observer->setVerbosityLevel(VerbosityLevel::eErrorsAndWarnings);
    observer->setPrintPrefix(
        BifrostUsd::GraphExecutor::kCtxGExecFactMakeGraphExecutor);

    // Ensure the library is initialized before accessing it:
    ensureLibrary();

    // LIMITATION: if a thread reaches this point while another thread just
    // called reloadLibrary(), then we would operate on an invalid library here.

    std::shared_lock<std::shared_mutex> libraryLock(getLibraryMutex());
    Amino::ConstLibrary&                lib = getLibrary();
    auto executable = lib.make_executable(observer, compound_name);
    return executable;
}

Amino::Executable getOrMakeExecutable_impl(Amino::StringView compound_name,
                                           StringArray*      errors) {
    // Step 1: check existence in the Registry under lock
    {
        std::lock_guard<std::mutex> registryLock(getRegistryMutex());
        auto&                       execReg = getExecutableRegistry();
        auto it = execReg.find(Amino::String{compound_name});
        if (it != execReg.end()) {
            return it->second.clone();
        }
    }

    // Step 2: Executable does not exist yet, create a new one.
    auto executable = makeExecutable_impl(compound_name, errors);
    if (!executable.isValid()) {
        return Amino::Executable{};
    }

    // Step 3: insert new Executable into cache.
    std::lock_guard<std::mutex> registryLock(getRegistryMutex());
    auto&                       execReg = getExecutableRegistry();

    // Re-check: another thread may have inserted it while we were creating
    // the Executable. Use try_emplace() to avoid overwriting an existing entry,
    // and does not move the Executable if another thread already inserted one.
    auto result = execReg.try_emplace(Amino::String{compound_name},
                                      std::move(executable));

    // Clone the element's value, let the newly created Executable be destroyed
    // if it was not inserted:
    return result.first->second.clone();
}

} // anonymous namespace

namespace BifrostUsd::GraphExecutor {

Amino::Executable makeExecutable(Amino::StringView compound_name) {
    return getOrMakeExecutable_impl(compound_name, nullptr);
}

Amino::Executable makeExecutable(Amino::StringView compound_name,
                                 StringArray&      errors) {
    errors.clear();
    return getOrMakeExecutable_impl(compound_name, &errors);
}

std::unique_ptr<GraphExecutor> makeGraphExecutor(
    Amino::StringView compound_name) {
    Amino::Executable executable =
        getOrMakeExecutable_impl(compound_name, nullptr);
    if (!executable.isValid()) {
        return nullptr;
    }
    return std::unique_ptr<GraphExecutor>(
        new GraphExecutor(std::move(executable)));
}

std::unique_ptr<GraphExecutor> makeGraphExecutor(
    Amino::StringView compound_name, StringArray& errors) {
    errors.clear();
    Amino::Executable executable =
        getOrMakeExecutable_impl(compound_name, &errors);
    if (!executable.isValid()) {
        return nullptr;
    }
    return std::unique_ptr<GraphExecutor>(
        new GraphExecutor(std::move(executable)));
}

// Reset the Library instance, clear the ExecutableRegistry, then reload the
// Library from the config files.
// LIMITATION: reloadLibrary() must not be called concurrently with
//      makeExecutable_impl(). Reloading is expected to happen only at startup
//      or on explicit user request, never during active execution.
void reloadLibrary() {
    {
        // Acquire an EXCLUSIVE lock to the registry and the library to safely
        // clear the registry and reset the library instance:
        std::scoped_lock<std::mutex, std::shared_mutex> locks(
            getRegistryMutex(), getLibraryMutex());

        getExecutableRegistry().clear();
        g_libInstance = Amino::ConstLibrary(); // replace by an invalid instance
    }
    // Ensure the library is immediately reloaded:
    ensureLibrary();
}

} // namespace BifrostUsd::GraphExecutor
