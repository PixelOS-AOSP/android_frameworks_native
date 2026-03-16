/*
 * Copyright 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <optional>

#include <scheduler/Fps.h>
#include <utils/Timers.h>

namespace android {

class OplusAdfr {
public:
    OplusAdfr() = default;
    ~OplusAdfr();

    Fps setFallbackRefreshRate(Fps fallbackRefreshRate);
    bool updateRefreshRate(Fps& refreshRate);

private:
    static constexpr nsecs_t kQueryIntervalNs = 150'000'000LL; // 150ms
    static constexpr int kMaxDisplayOpenRetries = 5;

    bool ensureDisplayReady();
    Fps resolveRefreshRate(Fps fallbackRefreshRate);

    nsecs_t mLastQueryNs = 0;
    std::optional<Fps> mFallbackRefreshRate;
    std::optional<Fps> mCachedFps;
    int mDisplayFd = -1;
    int mDisplayOpenAttempts = 0;
    bool mSkipNextAnimateResolve = false;
};

} // namespace android
