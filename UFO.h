#ifndef UFO_H
#define UFO_H

#include <glm/glm.hpp>
#include <CSCI441/objects.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class UFO {
public:
    UFO(GLuint shaderProgramHandle, GLint mvpMatrixLocation, GLint normalMatrixLocation,
        GLint materialAmbientLocation, GLint materialDiffuseLocation,
        GLint materialSpecularLocation, GLint materialShininessLocation);

    void drawUFO(glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void flyForward();
    void flyBackward();
    void turnLeft();
    void turnRight();

    glm::vec3 getPosition() const { return _position; }
    void setPosition(glm::vec3 &vec);
    float getBoundingRadius() const {return _boundingRadius; }
    float getHeading() const { return _heading; }
    void setPosition(const glm::vec3& pos) { _position = pos; }
    void setHeading(float heading) { _heading = heading; }

private:
    GLuint _shaderProgramHandle;
    GLint _mvpMatrixLocation;
    GLint _materialSpecularLocation;
    GLint _materialShininessLocation;
    GLint _normalMatrixLocation;
    GLint _materialAmbientLocation;
    GLint _materialDiffuseLocation;
    glm::vec3 _position;
    float _boundingRadius;
    float _heading;

    void drawCraft(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void drawLookingPort(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
};

#endif // UFO_H
