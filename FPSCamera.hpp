class FPSCamera {
public:
    FPSCamera(float heightOffset = 2.0f)
            : position(glm::vec3(0.0f)), direction(glm::vec3(0.0f, 0.0f, -1.0f)),
              up(glm::vec3(0.0f, 1.0f, 0.0f)), cameraHeightOffset(heightOffset) {}

    void setPosition(const glm::vec3& pos) { position = pos; }
    void setDirection(const glm::vec3& dir) { direction = glm::normalize(dir); }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + direction, up);
    }

    void updatePositionAndOrientation(const glm::vec3& vehiclePosition, float heading) {
        // Set the camera position with a height offset
        position = vehiclePosition + glm::vec3(0.0f, cameraHeightOffset, 0.0f);
        direction = glm::vec3(sin(heading), 0.0f, cos(heading));
    }

private:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up;
    float cameraHeightOffset;
};