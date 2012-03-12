#include "Component.h"
#include "Core.h"

#include <string.h>

namespace bgp
{
    struct Type
    {
        const char *mName;
        const ComponentDescriptor **mComponentDescriptors;
        size_t mNumComponents;
    };

    typedef void(ComponentCtor)(Component *, void *);

    struct ComponentDescriptor
    {
        const char *mName;
        bc::FixedBlockAllocator *mAllocator;
        ComponentCtor *mComponentCtor;
        // allocation/instantiation stuff here
    };

    static const size_t MAX_NUM_COMPONENT_DESCRIPTORS = 16;
    static ComponentDescriptor gRegisteredComponentDescriptors[MAX_NUM_COMPONENT_DESCRIPTORS];
    static size_t gNumComponentDescriptors;

    static const size_t MAX_NUM_TYPES = 16;
    static Type gRegisteredTypes[MAX_NUM_TYPES];
    static size_t gNumTypes;

    const ComponentDescriptor *GetComponentDescriptor(const char *name)
    {
        const ComponentDescriptor *descriptors = gRegisteredComponentDescriptors;

        for (size_t i = 0, e = gNumComponentDescriptors; i != e; ++i)
        {
            const ComponentDescriptor *descriptor = descriptors + i;
            if (strcmp(descriptor->mName, name) == 0)
                return descriptor;
        }

        return NULL;
    }

    const Type *GetType(const char *name)
    {
        const Type *types = gRegisteredTypes;

        for (size_t i = 0, e = gNumTypes; i != e; ++i)
        {
            const Type *type = types + i;
            if (strcmp(type->mName, name) == 0)
                return type;
        }

        return NULL;
    }

    const Type *CreateType(const char *name, const ComponentDescriptor **descriptors, size_t numComponents)
    {
        const size_t numTypes = gNumTypes;
        if (numTypes == MAX_NUM_TYPES)
            return NULL;

        Type *type = &gRegisteredTypes[numTypes];

        const size_t allocSize = numComponents * sizeof descriptors[0];
        const ComponentDescriptor **componentDescriptors = static_cast<const ComponentDescriptor **>(
            MemAlloc(bc::POOL_COMPONENT, allocSize));
        memcpy(componentDescriptors, descriptors, allocSize);

        type->mComponentDescriptors = componentDescriptors;
        type->mNumComponents = numComponents;
        type->mName = bc::Intern(NULL, NULL, name);

        return type;
    }

    GameObject *InstantiateType(const Type *type, ComponentInitializer *componentInitializers, size_t numInitializers)
    {
        const size_t numComponents = type->mNumComponents;
        const size_t allocSize = sizeof(GameObject) + numComponents * sizeof(Component *);
        GameObject *object = static_cast<GameObject *>(MemAlloc(bc::POOL_COMPONENT, allocSize));

        memset(&object->mPosition, 0, sizeof object->mPosition);
        object->mNumComponents = numComponents;
        object->mType = type;
        
        const ComponentDescriptor **componentDescriptors = type->mComponentDescriptors;
        for (size_t i = 0; i < numComponents; ++i)
        {
            const ComponentDescriptor *descriptor = componentDescriptors[i];
            Component *componentInstance = static_cast<Component *>(descriptor->mAllocator->Allocate());
            object->mComponents[i] = componentInstance;

            // Check if the user provided an initializer and pass it to the ctor fn
            for (size_t ii = 0; ii < numInitializers; ++ii)
            {
                if (componentInitializers[i].mComponentDescriptor == descriptor)
                {
                    descriptor->mComponentCtor(componentInstance, componentInitializers[i].mInitializerData);
                    goto nextComponent;
                }
            }

            // Otherwise, pass NULL.
            descriptor->mComponentCtor(componentInstance, NULL);

        nextComponent:
            ;
        }

        return object;
    }


#if 0
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
#endif
}