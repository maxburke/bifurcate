#include "Component.h"
#include "Core.h"

namespace bgp
{
    const size_t COMPONENT_ALIGNMENT = 16;

    void InitializeAllComponents()
    {
        SkinnedMeshComponent::Initialize(4);
        AnimatableComponent::Initialize(4);
    }

    Component::Component(GameObject *parent)
        : mParent(parent)
    {
        ++mGeneration;
    }

#define DEFINE_COMMON_COMPONENT_MEMBERS(COMPONENT) \
    bc::FixedBlockAllocator COMPONENT##Component::gComponentPool; \
    void COMPONENT##Component::Initialize(size_t poolSize) \
    { \
        gComponentPool.Initialize(poolSize, COMPONENT_ALIGNMENT, sizeof(COMPONENT ## Component)); \
    } \
    void *COMPONENT##Component::operator new(size_t size) \
    { \
        assert(size == sizeof(COMPONENT##Component)); \
        return gComponentPool.Allocate(); \
    } \
    void COMPONENT##Component::operator delete(void *obj) \
    { \
        gComponentPool.Free(obj); \
    }

    DEFINE_COMMON_COMPONENT_MEMBERS(SkinnedMesh)

    void SkinnedMeshComponent::UpdateAll()
    {
        for (size_t i = 0, e = gComponentPool.GetNumLiveObjects(); i < e; ++i)
        {
            SkinnedMeshComponent *obj = gComponentPool.GetObjectAtIndex<SkinnedMeshComponent>(i);
            UNUSED(obj);
        }
    }

    DEFINE_COMMON_COMPONENT_MEMBERS(Animatable)

    void AnimatableComponent::UpdateAll()
    {
        for (size_t i = 0, e = gComponentPool.GetNumLiveObjects(); i < e; ++i)
        {
            AnimatableComponent *obj = gComponentPool.GetObjectAtIndex<AnimatableComponent>(i);
            UNUSED(obj);
        }
    }
}