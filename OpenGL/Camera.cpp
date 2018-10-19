#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include "Camera.h"

using namespace glm;
CCamera::CCamera(vec3 position, vec3 up, float yaw, float pitch)
:_front(vec3(0.0f, 0.0f, -1.0f))
,_movement_speed(SPEED)
,_mouse_sensitivity(SENSITIVITY)
,_zoom(ZOOM)
,_position(position)
,_world_up(up)
,_yaw(yaw)
,_pitch(pitch)
{
	_update_camera_vectors();
}

CCamera::CCamera(
	float pos_x, 
	float pos_y, 
	float pos_z, 
	float up_x, 
	float up_y, 
	float up_z, 
	float yaw, 
	float pitch)
:_position(vec3(pos_x, pos_y, pos_z))
,_world_up(vec3(up_x, up_y, up_z))
,_yaw(yaw)
,_pitch(pitch)
{
	_update_camera_vectors();
}


CCamera::~CCamera()
{
}

mat4 CCamera::get_view_matrix()
{
	return lookAt(_position, _position + _front, _up);
}

void CCamera::_update_camera_vectors()
{
	vec3 front(1.0f);
	front.x = cos(radians(_yaw)) * cos(radians(_pitch));
	front.y = sin(radians(_pitch));
	front.z = sin(radians(_yaw)) * cos(radians(_pitch));

	_front = normalize(front);
	_right = normalize(cross(_front, _world_up));
	_up	   = normalize(cross(_right, _front));
}

void CCamera::on_keyboard_input(CameraMovement direction, float delta_time)
{
	float velocity = _movement_speed * delta_time;
	switch (direction)
	{
	case FORWARD:
		_position += _front * velocity;
		break;
	case BACKWARD:
		_position -= _front * velocity;
		break;
	case LEFT:
		_position -= _right * velocity;
		break;
	case RIGHT:
		_position += _right * velocity;
		break;
	}
}

void CCamera::on_mouse_move(float offset_x, float offset_y, GLboolean constrain_pitch)
{
	offset_x *= _mouse_sensitivity;
	offset_y *= _mouse_sensitivity;

	_yaw   += offset_x;
	_pitch += offset_y;

	if (constrain_pitch)
	{
		if (_pitch > 89.0f)
		{
			_pitch = 89.0f;
		}
		if (_pitch < -89.0f)
		{
			_pitch = -89.0f;
		}
	}

	_update_camera_vectors();
}

void CCamera::on_mouse_scroll(float offset_y)
{
	if (_zoom >= 1.0f && _zoom <= 45.0f)
	{
		_zoom -= offset_y;
	}
	if (_zoom <= 1.0f)
	{
		_zoom = 1.0f;
	}
	if (_zoom >= 45.0f)
	{
		_zoom = 45.0f;
	}
}
