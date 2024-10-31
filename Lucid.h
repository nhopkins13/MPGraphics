#ifndef A3_HERO_H
#define A3_HERO_H

#include <glm/glm.hpp>
#include <CSCI441/objects.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Lucid {
public:
    Lucid(GLuint shaderProgramHandle, GLint mvpMatrixLocation, GLint normalMatrixLocation,
          GLint materialAmbientLocation, GLint materialDiffuseLocation,
          GLint materialSpecularLocation, GLint materialShininessLocation);

    void drawLucid(glm::mat4 viewMtx, glm::mat4 projMtx) const;

    void move();
    void moveForward();
    void moveBackward();
    void turnLeft();
    void turnRight();

    glm::vec3 getPosition() const { return _position; }
    float getBoundingRadius() const {return _boundingRadius; }
    float getHeading() const { return _heading; }
    void setPosition(const glm::vec3& pos) { _position = pos; }
    void setHeading(float heading) { _heading = heading; }

private:
    GLuint _shaderProgramHandle;
    GLint _mvpMatrixLocation;
    GLint _normalMatrixLocation;
    GLint _materialAmbientLocation;
    GLint _materialDiffuseLocation;
    GLint _materialSpecularLocation;
    GLint _materialShininessLocation;

    glm::vec3 _position;
    float _boundingRadius;
    float _heading;

    float _wingAngle;

    const GLfloat _PI = glm::pi<float>();
    const GLfloat _2PI = glm::two_pi<float>();

    float _wingAngleRotationSpeed = _PI / 20.0f;
    float _rotateHeroAngle = _PI / 2.0f;

    void _drawUpperWing(bool isLeftWing, glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const;
    void _drawLowerWing(bool isLeftWing, glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const;
};

#endif
