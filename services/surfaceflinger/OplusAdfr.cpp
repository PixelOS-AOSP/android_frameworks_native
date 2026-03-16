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

#include "OplusAdfr.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace android {

namespace {

constexpr const char* kOplusDisplayPath = "/dev/oplus_display";
constexpr unsigned long kPanelIoctlGetDynamicTe = _IOWR('o', 0x5E, unsigned int);

} // namespace

OplusAdfr::~OplusAdfr() {
    if (mDisplayFd >= 0) {
        close(mDisplayFd);
    }
}

Fps OplusAdfr::setFallbackRefreshRate(Fps fallbackRefreshRate) {
    mFallbackRefreshRate = fallbackRefreshRate;
    mSkipNextAnimateResolve = true;
    return resolveRefreshRate(fallbackRefreshRate);
}

bool OplusAdfr::updateRefreshRate(Fps& refreshRate) {
    if (!mFallbackRefreshRate) {
        return false;
    }

    if (mSkipNextAnimateResolve) {
        mSkipNextAnimateResolve = false;
        return false;
    }

    const auto resolvedRefreshRate = resolveRefreshRate(*mFallbackRefreshRate);
    if (isApproxEqual(resolvedRefreshRate, refreshRate)) {
        return false;
    }

    refreshRate = resolvedRefreshRate;
    return true;
}

bool OplusAdfr::ensureDisplayReady() {
    if (mDisplayFd >= 0) {
        return true;
    }

    if (mDisplayOpenAttempts >= kMaxDisplayOpenRetries) {
        return false;
    }
    ++mDisplayOpenAttempts;

    mDisplayFd = open(kOplusDisplayPath, O_RDONLY | O_CLOEXEC);
    return mDisplayFd >= 0;
}

Fps OplusAdfr::resolveRefreshRate(Fps fallbackRefreshRate) {
    const nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    if (mLastQueryNs != 0 && now - mLastQueryNs < kQueryIntervalNs) {
        return mCachedFps.value_or(fallbackRefreshRate);
    }
    mLastQueryNs = now;

    if (!ensureDisplayReady()) {
        return mCachedFps.value_or(fallbackRefreshRate);
    }

    unsigned int refreshRate = 0;
    if (ioctl(mDisplayFd, kPanelIoctlGetDynamicTe, &refreshRate) < 0 || refreshRate == 0) {
        return mCachedFps.value_or(fallbackRefreshRate);
    }

    mCachedFps = Fps::fromValue(static_cast<int>(refreshRate));
    return *mCachedFps;
}

} // namespace android
