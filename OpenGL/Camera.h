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
		float pos_x,
		float pos_y,
		float pos_z,
		float up_x,
		float up_y,
		float up_z,
		float yaw,
		float pitch);

	~CCamera();

	glm::mat4 get_view_matrix();

	void on_keyboard_input(CameraMovement direction, float deltaTime);
	void on_mouse_move(float offset_x, float offset_y, GLboolean constrain_pitch = true);
	void on_mouse_scroll(float offset_y);

	float zoom() { return _zoom; }
private:
	void _update_camera_vectors();

	glm::vec3 _position;
	glm::vec3 _front;
	glm::vec3 _up;
	glm::vec3 _right;
	glm::vec3 _world_up;

	float _yaw;
	float _pitch;
	
	float _movement_speed;
	float _mouse_sensitivity;
	float _zoom;

};

#endif