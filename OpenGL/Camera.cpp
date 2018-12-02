#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include "Camera.h"

using namespace glm;
CCamera::CCamera(vec3 position, vec3 up, float yaw, float pitch)
:_front(vec3(0.0f, 0.0f, -1.0f))
,_movementSpeed(SPEED)
,_mouseSensitivity(SENSITIVITY)
,_zoom(ZOOM)
,_position(position)
,_worldUp(up)
,_yaw(yaw)
,_pitch(pitch)
{
	_updateCameraVectors();
}

CCamera::CCamera(
	float posX, 
	float posY, 
	float posZ, 
	float upX, 
	float upY, 
	float upZ, 
	float yaw, 
	float pitch)
:_position(vec3(posX, posY, posZ))
,_worldUp(vec3(upX, upY, upZ))
,_yaw(yaw)
,_pitch(pitch)
{
	_updateCameraVectors();
}


CCamera::~CCamera()
{
}

mat4 CCamera::getViewMatrix()
{
	return lookAt(_position, _position + _front, _up);
}

void CCamera::_updateCameraVectors()
{
	vec3 front(1.0f);
	front.x = cos(radians(_yaw)) * cos(radians(_pitch));
	front.y = sin(radians(_pitch));
	front.z = sin(radians(_yaw)) * cos(radians(_pitch));

	_front = normalize(front);
	_right = normalize(cross(_front, _worldUp));
	_up	   = normalize(cross(_right, _front));
}

void CCamera::onKeyboardInput(CameraMovement direction, float delta_time)
{
	float velocity = _movementSpeed * delta_time;
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

void CCamera::onMouseMove(float offset_x, float offset_y, GLboolean constrain_pitch)
{
	offset_x *= _mouseSensitivity;
	offset_y *= _mouseSensitivity;

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

	_updateCameraVectors();
}

void CCamera::onMouseScroll(float offset_y)
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
