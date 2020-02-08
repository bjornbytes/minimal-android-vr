#include <VrApi.h>
#include <VrApi_Input.h>

int main(int argc, char** argv) {
  ovrMobile* session;
  ovrHandPose pose;
  ovrHandSkeleton skeleton;
  ovrHandMesh mesh;
  vrapi_Initialize(NULL);
  vrapi_GetHandPose(session, 0, 0., &pose.Header);
  vrapi_GetHandSkeleton(session, VRAPI_HAND_LEFT, &skeleton.Header);
  vrapi_GetHandMesh(session, VRAPI_HAND_LEFT, &mesh.Header);
  return 0;
}
