#ifndef BIFURCATE_COMPONENT_H
#define BIFURCATE_COMPONENT_H

#include "Config.h"
#include "MathTypes.h"
#include "Gfx.h"
#include "Anim.h"
#include "Core.h"

namespace bgp
{
    typedef size_t Type;
    class GameObject;

    class Component
    {
    private:
         Component();

    protected:
        Component(GameObject *parent);
        ~Component();

        GameObject *mParent;
        uint32_t mGeneration;

        friend class ComponentHandle;
    };

    class ComponentHandle
    {
        Component *mComponent;
        uint32_t mGeneration;

    public:
        Component *ResolveHandle() 
        {
            assert(mGeneration == mComponent->mGeneration);
            return mComponent;
        }

        const Component *ResolveHandle() const
        {
            assert(mGeneration == mComponent->mGeneration);
            return mComponent;
        }
    };

    void InitializeAllComponents();

    class GameObject
    {
        bg::Vec3 mPosition;
        size_t mNumComponents;
        ComponentHandle *mComponents;

    public:
        size_t GetComponents(ComponentHandle *components, size_t componentArraySize);
        Component *GetComponent(Type type);
    };

#define DECLARE_COMMON_COMPONENT_MEMBERS() \
    static bc::FixedBlockAllocator gComponentPool; \
    static void UpdateAll(); \
    public: \
    static void Initialize(size_t poolSize); \
    void *operator new(size_t); \
    void operator delete(void *); \
    friend void InitializeAllComponents()

    class SkinnedMeshComponent : public Component
    {
        bg::Quaternion *mQuaternions;
        bg::MeshVertexBuffer *mMeshVertexBuffer;
        bg::MeshWeightedPositionBuffer *mWeightedPositionBuffer;
        bg::IndexBuffer *mIndexBuffer;
        bg::Material **mMaterials;

        DECLARE_COMMON_COMPONENT_MEMBERS();

    public:
        SkinnedMeshComponent(GameObject *object);
    };

    class AnimatableComponent : public Component
    {
        bg::AnimData *mClip;
        SkinnedMeshComponent *mMesh;

        DECLARE_COMMON_COMPONENT_MEMBERS();

    public:
        AnimatableComponent(GameObject *object);
    };
}

#endif