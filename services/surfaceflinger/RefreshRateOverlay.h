/*
 * Copyright 2019 The Android Open Source Project
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

#include "Utils/OverlayUtils.h"

#include <vector>

#include <ftl/flags.h>
#include <ftl/small_map.h>
#include <ui/LayerStack.h>
#include <ui/Size.h>
#include <ui/Transform.h>
#include <utils/StrongPointer.h>
#include <utils/Timers.h>

#include <scheduler/Fps.h>

class SkCanvas;

namespace android {

class GraphicBuffer;
class SurfaceFlinger;

class RefreshRateOverlay {
private:
    // Effectively making the constructor private, while keeping std::make_unique work
    struct ConstructorTag {};

public:
    enum class Features {
        Spinner = 1 << 0,
        RenderRate = 1 << 1,
        ShowInMiddle = 1 << 2,
        SetByHwc = 1 << 3,
    };

    static std::unique_ptr<RefreshRateOverlay> create(FpsRange, ftl::Flags<Features>);

    ~RefreshRateOverlay();

    void setLayerStack(ui::LayerStack);
    void setViewport(ui::Size);
    void changeRefreshRate(Fps, Fps);
    void changeRenderRate(Fps);
    void animate();
    bool isSetByHwc() const { return mFeatures.test(RefreshRateOverlay::Features::SetByHwc); }
    void onVrrIdle(bool idle);

    RefreshRateOverlay(ConstructorTag, FpsRange, ftl::Flags<Features>);

private:
    static constexpr nsecs_t kOplusQueryIntervalNs = 150'000'000LL; // 150ms
    static constexpr int kMaxOplusDisplayOpenRetries = 5;
    nsecs_t mLastOplusQueryNs = 0;
    std::optional<Fps> mCachedOplusFps;
    bool initCheck() const;

    using Buffers = std::vector<sp<GraphicBuffer>>;

    static Buffers draw(int refreshRate, int renderFps, bool idle, SkColor,
                        ui::Transform::RotationFlags, ftl::Flags<Features>);
    static void drawNumber(int number, int left, SkColor, SkCanvas&);
    static void drawDash(int left, SkCanvas&);

    const Buffers& getOrCreateBuffers(Fps, Fps, bool);

    SurfaceComposerClient::Transaction createTransaction() const;

    struct Key {
        int refreshRate;
        int renderFps;
        ui::Transform::RotationFlags flags;
        bool idle;

        bool operator==(Key other) const {
            return refreshRate == other.refreshRate && renderFps == other.renderFps &&
                    flags == other.flags && idle == other.idle;
        }
    };

    using BufferCache = ftl::SmallMap<Key, Buffers, 9>;
    BufferCache mBufferCache;

    std::optional<Fps> mRefreshRate;
    std::optional<Fps> mFallbackRefreshRate;
    std::optional<Fps> mRenderFps;
    int mOplusDisplayFd = -1;
    int mOplusDisplayOpenAttempts = 0;
    bool mSkipNextAnimateRefreshRateResolve = false;
    bool mIsVrrIdle = false;
    size_t mFrame = 0;

    const FpsRange mFpsRange; // For color interpolation.
    const ftl::Flags<Features> mFeatures;

    std::unique_ptr<SurfaceControlHolder> mSurfaceControl;

    Fps resolveRefreshRate(Fps);
    bool ensureOplusDisplayReady();
};

} // namespace android
