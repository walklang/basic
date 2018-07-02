// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROFILER_STACK_SAMPLING_PROFILER_H_
#define BASE_PROFILER_STACK_SAMPLING_PROFILER_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/atomicops.h"
#include "base/base_export.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"

namespace base {

// Identifies an unknown module.
BASE_EXPORT extern const size_t kUnknownModuleIndex;

class NativeStackSamplerTestDelegate;

// StackSamplingProfiler periodically stops a thread to sample its stack, for
// the purpose of collecting information about which code paths are
// executing. This information is used in aggregate by UMA to identify hot
// and/or janky code paths.
//
// Sample StackSamplingProfiler usage:
//
//   // Create and customize params as desired.
//   base::StackStackSamplingProfiler::SamplingParams params;
//
//   // To process the profiles, use a custom ProfileBuilder subclass:
//   class SubProfileBuilder :
//       public base::StackSamplingProfiler::ProfileBuilder{...}
//   base::StackSamplingProfiler profiler(base::PlatformThread::CurrentId()),
//       params, std::make_unique<SubProfileBuilder>(...));
//
//   profiler.Start();
//   // ... work being done on the target thread here ...
//   profiler.Stop();  // optional, stops collection before complete per params
//
// The default SamplingParams causes stacks to be recorded in a single profile
// at a 10Hz interval for a total of 30 seconds. All of these parameters may be
// altered as desired.
//
// When a call stack profile is complete, or the profiler is stopped,
// ProfileBuilder's OnProfileCompleted function is called from a thread created
// by the profiler.
class BASE_EXPORT StackSamplingProfiler {
 public:
  // Module represents the module (DLL or exe) corresponding to a stack frame.
  struct BASE_EXPORT Module {
    Module();
    Module(uintptr_t base_address,
           const std::string& id,
           const FilePath& filename);
    ~Module();

    // Points to the base address of the module.
    uintptr_t base_address;

    // An opaque binary string that uniquely identifies a particular program
    // version with high probability. This is parsed from headers of the loaded
    // module.
    // For binaries generated by GNU tools:
    //   Contents of the .note.gnu.build-id field.
    // On Windows:
    //   GUID + AGE in the debug image headers of a module.
    std::string id;

    // The filename of the module.
    FilePath filename;
  };

  // InternalModule represents the module (DLL or exe) and its validness state.
  // Different from Module, it has an additional field "is_valid".
  //
  // This struct is only used for sampling data transfer from NativeStackSampler
  // to ProfileBuilder.
  struct BASE_EXPORT InternalModule {
    InternalModule();
    InternalModule(uintptr_t base_address,
                   const std::string& id,
                   const FilePath& filename);
    ~InternalModule();

    // Points to the base address of the module.
    uintptr_t base_address;

    // An opaque binary string that uniquely identifies a particular program
    // version with high probability. This is parsed from headers of the loaded
    // module.
    // For binaries generated by GNU tools:
    //   Contents of the .note.gnu.build-id field.
    // On Windows:
    //   GUID + AGE in the debug image headers of a module.
    std::string id;

    // The filename of the module.
    FilePath filename;

    // The validness of the module.
    bool is_valid;
  };

  // Frame represents an individual sampled stack frame with module information.
  struct BASE_EXPORT Frame {
    Frame(uintptr_t instruction_pointer, size_t module_index);
    ~Frame();

    // Default constructor to satisfy IPC macros. Do not use explicitly.
    Frame();

    // The sampled instruction pointer within the function.
    uintptr_t instruction_pointer;

    // Index of the module in CallStackProfile::modules. We don't represent
    // module state directly here to save space.
    size_t module_index;
  };

  // InternalFrame represents an individual sampled stack frame with full module
  // information. This is different from Frame which only contains module index.
  //
  // This struct is only used for sampling data transfer from NativeStackSampler
  // to ProfileBuilder.
  struct BASE_EXPORT InternalFrame {
    InternalFrame(uintptr_t instruction_pointer,
                  InternalModule internal_module);
    ~InternalFrame();

    // The sampled instruction pointer within the function.
    uintptr_t instruction_pointer;

    // The module information.
    InternalModule internal_module;
  };

  // Sample represents a set of stack frames with some extra information.
  struct BASE_EXPORT Sample {
    Sample();
    Sample(const Sample& sample);
    ~Sample();

    // These constructors are used only during testing.
    Sample(const Frame& frame);
    Sample(const std::vector<Frame>& frames);

    // The entire stack frame when the sample is taken.
    std::vector<Frame> frames;

    // A bit-field indicating which process milestones have passed. This can be
    // used to tell where in the process lifetime the samples are taken. Just
    // as a "lifetime" can only move forward, these bits mark the milestones of
    // the processes life as they occur. Bits can be set but never reset. The
    // actual definition of the individual bits is left to the user of this
    // module.
    uint32_t process_milestones = 0;
  };

  // CallStackProfile represents a set of samples.
  struct BASE_EXPORT CallStackProfile {
    CallStackProfile();
    CallStackProfile(CallStackProfile&& other);
    ~CallStackProfile();

    CallStackProfile& operator=(CallStackProfile&& other);

    CallStackProfile CopyForTesting() const;

    std::vector<Module> modules;
    std::vector<Sample> samples;

    // Duration of this profile.
    TimeDelta profile_duration;

    // Time between samples.
    TimeDelta sampling_period;

   private:
    // Copying is possible but expensive so disallow it except for internal use
    // (i.e. CopyForTesting); use std::move instead.
    CallStackProfile(const CallStackProfile& other);

    DISALLOW_ASSIGN(CallStackProfile);
  };

  // Represents parameters that configure the sampling.
  struct BASE_EXPORT SamplingParams {
    // Time to delay before first samples are taken.
    TimeDelta initial_delay = TimeDelta::FromMilliseconds(0);

    // Number of samples to record per profile.
    int samples_per_profile = 300;

    // Interval between samples during a sampling profile. This is the desired
    // duration from the start of one sample to the start of the next sample.
    TimeDelta sampling_interval = TimeDelta::FromMilliseconds(100);
  };

  // Testing support. These methods are static beause they interact with the
  // sampling thread, a singleton used by all StackSamplingProfiler objects.
  // These methods can only be called by the same thread that started the
  // sampling.
  class BASE_EXPORT TestAPI {
   public:
    // Resets the internal state to that of a fresh start. This is necessary
    // so that tests don't inherit state from previous tests.
    static void Reset();

    // Resets internal annotations (like process phase) to initial values.
    static void ResetAnnotations();

    // Returns whether the sampling thread is currently running or not.
    static bool IsSamplingThreadRunning();

    // Disables inherent idle-shutdown behavior.
    static void DisableIdleShutdown();

    // Initiates an idle shutdown task, as though the idle timer had expired,
    // causing the thread to exit. There is no "idle" check so this must be
    // called only when all sampling tasks have completed. This blocks until
    // the task has been executed, though the actual stopping of the thread
    // still happens asynchronously. Watch IsSamplingThreadRunning() to know
    // when the thread has exited. If |simulate_intervening_start| is true then
    // this method will make it appear to the shutdown task that a new profiler
    // was started between when the idle-shutdown was initiated and when it
    // runs.
    static void PerformSamplingThreadIdleShutdown(
        bool simulate_intervening_start);
  };

  // The ProfileBuilder interface allows the user to record profile information
  // on the fly in whatever format is desired. Functions are invoked by the
  // profiler on its own thread so must not block or perform expensive
  // operations.
  class BASE_EXPORT ProfileBuilder {
   public:
    ProfileBuilder() = default;
    virtual ~ProfileBuilder() = default;

    // Metadata associated with the sample to be saved off.
    // The code implementing this method must not do anything that could acquire
    // a mutex, including allocating memory (which includes LOG messages)
    // because that mutex could be held by a stopped thread, thus resulting in
    // deadlock.
    virtual void RecordAnnotations() = 0;

    // Records a new set of internal frames. Invoked when sampling a sample
    // completes.
    virtual void OnSampleCompleted(
        std::vector<InternalFrame> internal_frames) = 0;

    // Finishes the profile construction with |profile_duration| and
    // |sampling_period|. Invoked when sampling a profile completes.
    virtual void OnProfileCompleted(TimeDelta profile_duration,
                                    TimeDelta sampling_period) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(ProfileBuilder);
  };

  // The callback type used to collect a completed profile. The passed |profile|
  // is move-only. Other threads, including the UI thread, may block on callback
  // completion so this should run as quickly as possible.
  //
  // IMPORTANT NOTE: The callback is invoked on a thread the profiler
  // constructs, rather than on the thread used to construct the profiler, and
  // thus the callback must be callable on any thread. For threads with message
  // loops that create StackSamplingProfilers, posting a task to the message
  // loop with the moved (i.e. std::move) profile is the thread-safe callback
  // implementation.
  using CompletedCallback = Callback<void(CallStackProfile)>;

  // Creates a profiler for the CURRENT thread. An optional |test_delegate| can
  // be supplied by tests. The caller must ensure that this object gets
  // destroyed before the current thread exits.
  StackSamplingProfiler(
      const SamplingParams& params,
      std::unique_ptr<ProfileBuilder> profile_builder,
      NativeStackSamplerTestDelegate* test_delegate = nullptr);

  // Creates a profiler for ANOTHER thread. An optional |test_delegate| can be
  // supplied by tests.
  //
  // IMPORTANT: The caller must ensure that the thread being sampled does not
  // exit before this object gets destructed or Bad Things(tm) may occur.
  StackSamplingProfiler(
      PlatformThreadId thread_id,
      const SamplingParams& params,
      std::unique_ptr<ProfileBuilder> profile_builder,
      NativeStackSamplerTestDelegate* test_delegate = nullptr);

  // Stops any profiling currently taking place before destroying the profiler.
  // This will block until profile_builder_'s OnProfileCompleted function has
  // executed if profiling has started but not already finished.
  ~StackSamplingProfiler();

  // Initializes the profiler and starts sampling. Might block on a
  // WaitableEvent if this StackSamplingProfiler was previously started and
  // recently stopped, while the previous profiling phase winds down.
  void Start();

  // Stops the profiler and any ongoing sampling. This method will return
  // immediately with the profile_builder_'s OnProfileCompleted function being
  // run asynchronously. At most one more stack sample will be taken after this
  // method returns. Calling this function is optional; if not invoked profiling
  // terminates when all the profiling samples specified in the SamplingParams
  // are completed or the profiler object is destroyed, whichever occurs first.
  void Stop();

  // Sets the current system state that is recorded with each captured stack
  // frame. This is thread-safe so can be called from anywhere. The parameter
  // value should be from an enumeration of the appropriate type with values
  // ranging from 0 to 31, inclusive. This sets bits within Sample field of
  // |process_milestones|. The actual meanings of these bits are defined
  // (globally) by the caller(s).
  static void SetProcessMilestone(int milestone);

  // Gets the current system state that is recorded with each captured stack
  // frame. This is thread-safe so can be called from anywhere.
  static subtle::Atomic32 ProcessMilestone();

 private:
  friend class TestAPI;

  // SamplingThread is a separate thread used to suspend and sample stacks from
  // the target thread.
  class SamplingThread;

  // This global variables holds the current system state and is recorded with
  // every captured sample, done on a separate thread which is why updates to
  // this must be atomic. A PostTask to move the the updates to that thread
  // would skew the timing and a lock could result in deadlock if the thread
  // making a change was also being profiled and got stopped.
  static subtle::Atomic32 process_milestones_;

  // The thread whose stack will be sampled.
  PlatformThreadId thread_id_;

  const SamplingParams params_;

  // Receives the sampling data and builds a CallStackProfile. The ownership of
  // this object will be transferred to the sampling thread when thread sampling
  // starts.
  std::unique_ptr<ProfileBuilder> profile_builder_;

  // This starts "signaled", is reset when sampling begins, and is signaled
  // when that sampling is complete and the profile_builder_'s
  // OnProfileCompleted function has executed.
  WaitableEvent profiling_inactive_;

  // An ID uniquely identifying this profiler to the sampling thread. This
  // will be an internal "null" value when no collection has been started.
  int profiler_id_;

  // Stored until it can be passed to the NativeStackSampler created in Start().
  NativeStackSamplerTestDelegate* const test_delegate_;

  DISALLOW_COPY_AND_ASSIGN(StackSamplingProfiler);
};

// These operators permit types to be compared and used in a map of Samples, as
// done in tests and by the metrics provider code.
BASE_EXPORT bool operator==(const StackSamplingProfiler::Module& a,
                            const StackSamplingProfiler::Module& b);
BASE_EXPORT bool operator==(const StackSamplingProfiler::Sample& a,
                            const StackSamplingProfiler::Sample& b);
BASE_EXPORT bool operator!=(const StackSamplingProfiler::Sample& a,
                            const StackSamplingProfiler::Sample& b);
BASE_EXPORT bool operator<(const StackSamplingProfiler::Sample& a,
                           const StackSamplingProfiler::Sample& b);
BASE_EXPORT bool operator==(const StackSamplingProfiler::Frame& a,
                            const StackSamplingProfiler::Frame& b);
BASE_EXPORT bool operator<(const StackSamplingProfiler::Frame& a,
                           const StackSamplingProfiler::Frame& b);

}  // namespace base

#endif  // BASE_PROFILER_STACK_SAMPLING_PROFILER_H_
