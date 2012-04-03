#include <string.h>

#include "Core.h"
#include "Camera.h"
#include "MathTypes.h"
#include "SseMath.h"
#include "Controller.h"

namespace bg
{
    struct FreeCam
    {
        Mat4x4 mView;
        Vec2 mRotation;
        Vec3 mPosition;
        Quaternion mQuaternion;
        int mHalfWindowWidth;
        int mHalfWindowHeight;
    };

    FreeCam *FreeCamCreate(int windowWidth, int windowHeight)
    {
        FreeCam *freeCam = static_cast<FreeCam *>(MemAlloc(bc::POOL_SMALL, sizeof(FreeCam)));
        memset(freeCam, 0, sizeof(FreeCam));
        freeCam->mView.SetIdentity();
        freeCam->mHalfWindowHeight = windowHeight / 2;
        freeCam->mHalfWindowWidth = windowWidth / 2;

        return freeCam;
    }

    void FreeCamDestroy(FreeCam *freeCam)
    {
        bc::MemFree(freeCam);
    }

    static void UpdateViewRotation(FreeCam *freeCam)
    {
        const bc::Controller *controller = bc::ControllerGetState(bc::CONTROLLER_KEYBOARD_MOUSE);

        __m128 dx = _mm_set_ss(-controller->mRightX);
        __m128 dy = _mm_set_ss(-controller->mRightY);

        const __m128 ROTATION_SPEED = _mm_set_ss(0.25f);
        __m128 rotX = _mm_add_ss(_mm_set_ss(freeCam->mRotation.x), _mm_mul_ss(dx, ROTATION_SPEED));
        __m128 rotY = _mm_add_ss(_mm_set_ss(freeCam->mRotation.y), _mm_mul_ss(dy, ROTATION_SPEED));

        const __m128 Y_LIMIT = _mm_set_ss(1.4f);
        const __m128 X_LIMIT = _mm_set_ss(3.1415926535f);
        rotX = _mm_clamp_ps(rotX, _mm_neg_ps(X_LIMIT), X_LIMIT);
        rotY = _mm_clamp_ps(rotY, _mm_neg_ps(Y_LIMIT), Y_LIMIT);

        _mm_store_ss(&freeCam->mRotation.x, rotX);
        _mm_store_ss(&freeCam->mRotation.y, rotY);
    }

    static void UpdatePosition(FreeCam *freeCam)
    {
        // TODO: This will probably need to be updated if/when a central control scheme 
        // management suite is built.
        Vec3 displacement = {};

        const bc::Controller *controller = bc::ControllerGetState(bc::CONTROLLER_KEYBOARD_MOUSE);

        displacement.z -= controller->mLeftY;
        displacement.x += controller->mLeftX;

        if (displacement.x == 0 && displacement.z == 0)
            return;

        __m128 v = _mm_load_ps(&displacement.v[0]);
        __m128 lengthSquared = _mm_dot_ps(v, v);
        __m128 normalized = _mm_mul_ps(v, _mm_rsqrt_ps(lengthSquared));

        Mat4x4 rotation;
        bg::Mat4x4FromQuaternion(&rotation, &freeCam->mQuaternion);
        
        const float scale = 4.f;
        __m128 scaledDisplacement = _mm_mul_ps(normalized, _mm_set1_ps(scale));
        __m128 xDispTransformed = _mm_dotps_ss(scaledDisplacement, _mm_load_ps(&rotation.v[0]));
        __m128 yDispTransformed = _mm_dotps_ss(scaledDisplacement, _mm_load_ps(&rotation.v[4]));
        __m128 zDispTransformed = _mm_dotps_ss(scaledDisplacement, _mm_load_ps(&rotation.v[8]));

        _mm_store_ss(&freeCam->mPosition.x, _mm_add_ps(xDispTransformed, _mm_load_ss(&freeCam->mPosition.x)));
        _mm_store_ss(&freeCam->mPosition.y, _mm_add_ps(yDispTransformed, _mm_load_ss(&freeCam->mPosition.y)));
        _mm_store_ss(&freeCam->mPosition.z, _mm_add_ps(zDispTransformed, _mm_load_ss(&freeCam->mPosition.z)));
    }

    static void GenerateViewMatrix(FreeCam *freeCam)
    {
        Vec3 x = { 0, -1, 0 };
        Vec3 y = { -1, 0, 0 };

        Quaternion xRotationQuaternion = QuaternionFromAxisAngle(&x, freeCam->mRotation.x);
        Quaternion yRotationQuaternion = QuaternionFromAxisAngle(&y, freeCam->mRotation.y);
        Quaternion cameraQuaternion = QuaternionMultiply(&xRotationQuaternion, &yRotationQuaternion);
        QuatPos cameraQuatPos;
        cameraQuatPos.quat = cameraQuaternion;
        cameraQuatPos.pos = freeCam->mPosition;

        Mat4x4 newViewMatrix;
        Mat4x4FromQuatPos(&newViewMatrix, &cameraQuatPos);
        Mat4x4Invert(&freeCam->mView, &newViewMatrix);
        freeCam->mQuaternion = cameraQuaternion;
    }

    void FreeCamUpdate(FreeCam *freeCam)
    {
        UpdateViewRotation(freeCam);
        UpdatePosition(freeCam);
        GenerateViewMatrix(freeCam);
    }

    void FreeCamFetchViewMatrix(const FreeCam *freeCam, Mat4x4 *out)
    {
        *out = freeCam->mView;
    }
}