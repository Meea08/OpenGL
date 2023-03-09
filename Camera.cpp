#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition); 
        this->cameraRightDirection = glm::normalize(glm::cross(cameraUpDirection, cameraFrontDirection));
        this->cameraUpDirection = glm::cross(cameraFrontDirection, cameraRightDirection);
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        switch (direction) {
        case MOVE_FORWARD:
            cameraPosition += cameraFrontDirection * speed;
            break;
        case MOVE_BACKWARD:
            cameraPosition -= cameraFrontDirection * speed;
            break;
        case MOVE_LEFT:
            cameraPosition += cameraRightDirection * speed;
            break;
        case MOVE_RIGHT:
            cameraPosition -= cameraRightDirection * speed;
            break;
        case MOVE_UP:
            cameraPosition += cameraUpDirection * speed;
            break;
        case MOVE_DOWN:
            cameraPosition -= cameraUpDirection * speed;
            break;
        }
        this->cameraTarget = this->cameraFrontDirection + this->cameraPosition;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        this->cameraTarget.x = this->cameraPosition.x + cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->cameraTarget.y = this->cameraPosition.y + sin(glm::radians(pitch));
        this->cameraTarget.z = this->cameraPosition.z - sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        this->cameraFrontDirection = glm::normalize(this->cameraTarget - this->cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), cameraFrontDirection));
        this->cameraUpDirection = glm::cross(cameraFrontDirection, cameraRightDirection);
        
    }
    void Camera::printPosition() {
        printf("position: %f %f %f\n", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    }
    void Camera::printTarget() {
        printf("target: %f %f %f\n", cameraTarget.x, cameraTarget.y, cameraTarget.z);
    }
    void Camera::moveTo(glm::vec3 newPos, glm::vec3 newTar) {
        this->cameraPosition = newPos;
        this->cameraTarget = newTar;
    }
}