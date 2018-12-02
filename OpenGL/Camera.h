#ifndef __C_CAMERA_H__
#define __C_CAMERA_H__

enum CameraMovement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
};

const float YAW			= -90.0f;
const float PITCH		= 0.0f;
const float SPEED		= 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM		= 45.0f;

class CCamera
{
public:
	CCamera(
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
		float yaw = YAW,
		float pitch = PITCH);

	CCamera(
		float posX,
		float posY,
		float posZ,
		float upX,
		float upY,
		float upZ,
		float yaw,
		float pitch);

	~CCamera();

	glm::mat4 getViewMatrix();

	void onKeyboardInput(CameraMovement direction, float deltaTime);
	void onMouseMove(float offsetX, float offsetY, GLboolean constrainPitch = true);
	void onMouseScroll(float offsetY);

	float zoom() { return _zoom; }
private:
	void _updateCameraVectors();

	glm::vec3 _position;
	glm::vec3 _front;
	glm::vec3 _up;
	glm::vec3 _right;
	glm::vec3 _worldUp;

	float _yaw;
	float _pitch;
	
	float _movementSpeed;
	float _mouseSensitivity;
	float _zoom;

};

#endif