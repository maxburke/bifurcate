#ifndef BIFURCATE_CAMERA_H
#define BIFURCATE_CAMERA_H

namespace bg
{
    struct Mat4x4;
    struct FreeCam;

    FreeCam *FreeCamCreate(int windowWidth, int windowHeight);
    void FreeCamDestroy(FreeCam *freeCam);
    void FreeCamUpdate(FreeCam *freeCam);
    void FreeCamFetchViewMatrix(const FreeCam *freeCam, Mat4x4 *out);
}

#endif