#pragma once
#include "Animation.h"
/**
 * @brief Rotates an object at a continuous rate over an interval.
 */
class RotationAnimation : public Animation {
private:
	/**
	 * @brief How much to increment the orientation by each second.
	 */
	glm::vec3 m_perSecond;
	glm::vec3& m_objectOrientation;

	/**
	 * @brief Advance the animation by the given time interval.
	 */
	void applyAnimation(float dt) override {
		m_objectOrientation += m_perSecond * dt;
	}

public:
	/**
	 * @brief Constructs a animation of a constant rotation by the given total rotation
	 * angle, linearly interpolated across the given duration.
	 */
	RotationAnimation(glm::vec3& objectOrientation, float duration, const glm::vec3& totalRotation) :
		Animation{ duration }, m_objectOrientation{ objectOrientation }, m_perSecond{ totalRotation / duration } {}
};

