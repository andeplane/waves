#pragma once
#include <QtGui/QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <vector>

struct VertexData
{
    QVector3D position;
    QVector3D color;
    QVector2D textureCoord;
};

class CPGLQuads
{
public:
    CPGLQuads();
    ~CPGLQuads();
    void update(std::vector<QVector3D> &positions, std::vector<QVector3D> colors = {}, std::vector<float> scalings = {});
    void render(float lightFalloffDistance, const QMatrix4x4 &modelViewProjectionMatrix, const QMatrix4x4 &lightModelViewProjectionMatrix);
    void setModelViewMatrix(QMatrix4x4& matrix);
private:
    GLuint m_vboIds[2];
    std::vector<VertexData> m_vertices;
    std::vector<GLushort> m_indices;
    QOpenGLFunctions *m_funcs;
    QMatrix4x4 m_modelViewMatrix;
    QOpenGLShaderProgram *m_program;

    void createShaderProgram();
    void generateVBOs();
    void ensureInitialized();
    QVector3D vectorFromColor(const QColor &color);
};
