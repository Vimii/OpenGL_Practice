#include "Camera.h"


Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED_NORMAL), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED_NORMAL), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(Camera_Key_Input input, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;
    if (input == FORWARD)
        Position += Front * velocity;
    if (input == BACKWARD)
        Position -= Front * velocity;
    if (input == LEFT)
        Position -= Right * velocity;
    if (input == RIGHT)
        Position += Right * velocity;
    if (input == DOWN)
        Position -= Up * velocity;
    if (input == UP)
        Position += Up * velocity;
}

void Camera::SpeedManager(bool dash)
{
    if (dash) MovementSpeed = SPEED_DASH;
    else MovementSpeed = SPEED_NORMAL;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;


    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::SetZoom(float zoom)
{
    Zoom = zoom;
}

void Camera::ProcessMouseScroll(float yoffset)
{
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 90.0f)
        Zoom = 90.0f;
}

void Camera::updateCameraVectors()
{
   glm::vec3 front;
   front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
   front.y = sin(glm::radians(Pitch));
   front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
   Front = glm::normalize(front);
   Right = glm::normalize(glm::cross(Front, WorldUp)); 
   Up = glm::normalize(glm::cross(Right, Front));
}