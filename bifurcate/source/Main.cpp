#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <stdio.h>
#pragma warning(pop)

#include "Config.h"
#include "Core.h"
#include "Gfx.h"
#include "Anim.h"
#include "Mesh.h"

#include "Component.h"

void RunTests();

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdline, int showCmd)
{
    UNUSED(instance);
    UNUSED(prevInstance);
    UNUSED(cmdline);
    UNUSED(showCmd);

    // Initialize frame times to some value that isn't nonsense.
    bc::UpdateFrameTime();

    if (!bg::GfxInitialize(instance, 1280, 720))
        return 1;

    RunTests();

    //bx::load_map("z:/doom3data/maps/game/recycling1.proc");
    const bg::AnimData *ad = bg::LoadAnim("z:/doom3data/models/md5/monsters/hellknight/walk7.md5anim");
    const bg::SkinnedMeshData *md = bg::LoadMesh("z:/doom3data/models/md5/monsters/hellknight/hellknight.md5mesh");

    MSG msg;

    int mFrame0 = 0;
    int mFrame1 = 0;
    float mCurrentLerpFactor = 0;

#define WRAP(X, MIN, MAX) (((X)>=(MAX))?(MIN):(X))

    const int elements = ad->mBaseFrame.mNumElements;
    void *memory = AllocaAligned(SIMD_ALIGNMENT, bg::SoaQuatPos::MemorySize(elements));

    while (GetMessage(&msg, NULL, 0, 0))
    {
        bc::UpdateFrameTime();

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // The code between the two comments below is essentially the animatable component
        // update loop. Fantastic!
        // void AnimatableComponent::Update(const bg::AnimData *ad) {
            bg::SoaQuatPos out;

            // These two lines below are commented out because otherwise we leak stack memory
            // and explode. BOOM. But, otherwise, we would use them as in the commented-out version
            // in AnimatableComponent::Update
            // const int elements = ad->mBaseFrame.mNumElements;
            // out.Initialize(elements, AllocaAligned(SIMD_ALIGNMENT, bg::SoaQuatPos::MemorySize(elements)));
            out.Initialize(elements, memory);

            int numFramesPerAnimation = ad->mNumFrames;
            int frame0 = mFrame0;
            int frame1 = mFrame1;
            float lerpFactor = static_cast<float>(bc::GetFrameTicks()) * ad->mInvClockTicksPerFrame;
            float currentLerpFactor = mCurrentLerpFactor + lerpFactor;

            while (currentLerpFactor > 1.0f)
            {
                frame0 = WRAP(frame0 + 1, 0, numFramesPerAnimation);
                frame1 = WRAP(frame0 + 1, 0, numFramesPerAnimation);
                currentLerpFactor -= 1.0f;
            }

            bg::InterpolateAnimationFrames(&out, ad, frame0, frame1, currentLerpFactor);

            mFrame0 = frame0;
            mFrame1 = frame1;
            mCurrentLerpFactor = currentLerpFactor;
        // }

        UNUSED(md);
        bg::GfxBeginScene();
        bg::DrawSkinnedMesh(md, &out);
        bg::GfxEndScene();
    }

    bg::GfxShutdown();

    return 0;
}
