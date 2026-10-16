#include "objects/Sun.h"

#include <cmath>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr double kPi = 3.14159265358979323846;
} // namespace

#define SUN_ANGLE_STEP 0.001
#define SUN_YAW_OFFSET kPi / 10.0
#define SUN_YAW_START -SUN_YAW_OFFSET
#define SUN_YAW_END kPi + SUN_YAW_OFFSET

namespace {
// Sun light level, tuned below unity: at 1.0 the direct sun outranges
// ambient/fog by ~200x in shade views, forcing the auto-exposure to
// its ceiling and clipping sky and sunlit leaves.
constexpr float kSunIntensity = 0.7f;

// Zero-length maps to zero, never NaN.
glm::vec3 safeNormalize(const glm::vec3 &v) {
  float len = glm::length(v);
  if (len > std::numeric_limits<float>::epsilon())
    return v / len;
  return glm::vec3(0.0f);
}
} // namespace

Sun::Sun(std::unique_ptr<MaterialSun> mat) : Mesh() {
  geometry = Geometries::quad(2.0);
  material = std::move(mat);
  light = std::make_unique<Light>();
  // Tuned below unity: at 1.0 the direct sun outranges ambient/fog by
  // ~200x in shade views, forcing the auto-exposure to its ceiling and
  // clipping sky and sunlit leaves. kSunIntensity keeps sunlit
  // hierarchy without the blowout; revisit with the exposure log if
  // shade goes too dark.
  light->setIntensity(glm::vec3(kSunIntensity));
  lightDistance = 150.0;
  lightTargetOffset = 13;
  sunDistance = 6.0;
  yaw = static_cast<float>(kPi / 3.0);
  speed = 0.0;
}

void Sun::update(const glm::vec3 &positionCamera, const glm::vec3 &zCamera) {
  yaw += static_cast<float>(SUN_ANGLE_STEP * speed);
  if (yaw > SUN_YAW_END)
    yaw = static_cast<float>(SUN_YAW_START);

  localPosition = glm::vec3(static_cast<float>(std::cos(yaw)),
                            static_cast<float>(std::sin(yaw)),
                            static_cast<float>(-std::sin(yaw) / 3.0));
  position = positionCamera + localPosition * sunDistance;

  // Object-orientation basis (NOT the view matrix): rotation columns
  // (x, y, -z), translation (eye). The zero forward guard mirrors the
  // historical z.z = 0.1 fallback.
  glm::vec3 zAxis = positionCamera - position;
  if (glm::length(zAxis) <= std::numeric_limits<float>::epsilon())
    zAxis = glm::vec3(0.0f, 0.0f, 0.1f);
  else
    zAxis = glm::normalize(zAxis);
  glm::vec3 xAxis = safeNormalize(glm::cross(zAxis, glm::vec3(0, 1, 0)));
  glm::vec3 yAxis = safeNormalize(glm::cross(xAxis, zAxis));
  matrix = glm::mat4(glm::vec4(xAxis, 0.0f), glm::vec4(yAxis, 0.0f),
                     glm::vec4(-zAxis, 0.0f), glm::vec4(position, 1.0f));

  glm::vec3 offsetCamera = positionCamera + zCamera * lightTargetOffset;
  light->setPosition(offsetCamera + localPosition * lightDistance);
  light->lookAt(offsetCamera);
}

void Sun::setSpeed(float s) { speed = s; }

Sun::~Sun() {
}

Light *Sun::getLight() { return light.get(); }
