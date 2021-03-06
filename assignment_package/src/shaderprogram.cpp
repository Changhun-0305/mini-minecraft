#include "shaderprogram.h"
#include <QFile>
#include <QStringBuilder>
#include <QTextStream>
#include <QDebug>
#include <stdexcept>
#include "iostream"


ShaderProgram::ShaderProgram(OpenGLContext *context)
    : vertShader(), fragShader(), prog(),
      attrPos(-1), attrNor(-1), attrCol(-1), attrUv(-1), animate(-1),
      unifModel(-1), unifModelInvTr(-1), unifViewProj(-1), unifColor(-1),
      unifSampler2D(-1), unifTime(-1), context(context)
{}

void ShaderProgram::create(const char *vertfile, const char *fragfile)
{
    // Allocate space on our GPU for a vertex shader and a fragment shader and a shader program to manage the two
    vertShader = context->glCreateShader(GL_VERTEX_SHADER);
    fragShader = context->glCreateShader(GL_FRAGMENT_SHADER);
    prog = context->glCreateProgram();
    // Get the body of text stored in our two .glsl files
    QString qVertSource = qTextFileRead(vertfile);
    QString qFragSource = qTextFileRead(fragfile);

    char* vertSource = new char[qVertSource.size()+1];
    strcpy(vertSource, qVertSource.toStdString().c_str());
    char* fragSource = new char[qFragSource.size()+1];
    strcpy(fragSource, qFragSource.toStdString().c_str());


    // Send the shader text to OpenGL and store it in the shaders specified by the handles vertShader and fragShader
    context->glShaderSource(vertShader, 1, &vertSource, 0);
    context->glShaderSource(fragShader, 1, &fragSource, 0);
    // Tell OpenGL to compile the shader text stored above
    context->glCompileShader(vertShader);
    context->glCompileShader(fragShader);
    // Check if everything compiled OK
    GLint compiled;
    context->glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderInfoLog(vertShader);
    }
    context->glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderInfoLog(fragShader);
    }

    // Tell prog that it manages these particular vertex and fragment shaders
    context->glAttachShader(prog, vertShader);
    context->glAttachShader(prog, fragShader);
    context->glLinkProgram(prog);

    // Check for linking success
    GLint linked;
    context->glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        printLinkInfoLog(prog);
    }

    // Get the handles to the variables stored in our shaders
    // See shaderprogram.h for more information about these variables

    attrPos = context->glGetAttribLocation(prog, "vs_Pos");
    attrNor = context->glGetAttribLocation(prog, "vs_Nor");
    attrUv = context->glGetAttribLocation(prog, "vs_UV");
    animate = context->glGetAttribLocation(prog, "vs_animate");

    unifModel      = context->glGetUniformLocation(prog, "u_Model");
    unifModelInvTr = context->glGetUniformLocation(prog, "u_ModelInvTr");
    unifViewProj   = context->glGetUniformLocation(prog, "u_ViewProj");
    unifColor      = context->glGetUniformLocation(prog, "u_Color");
    unifSampler2D = context->glGetUniformLocation(prog, "u_Texture");
    unifTime = context->glGetUniformLocation(prog, "u_Time");
    // Sky demo
    unifDimensions = context->glGetUniformLocation(prog, "u_Dimensions");
    unifEye = context->glGetUniformLocation(prog, "u_Eye");

}

void ShaderProgram::useMe()
{
    context->glUseProgram(prog);
}

void ShaderProgram::setModelMatrix(const glm::mat4 &model)
{
    useMe();

    if (unifModel != -1) {
        // Pass a 4x4 matrix into a uniform variable in our shader
                        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(unifModel,
                        // How many matrices to pass
                           1,
                        // Transpose the matrix? OpenGL uses column-major, so no.
                           GL_FALSE,
                        // Pointer to the first element of the matrix
                           &model[0][0]);
    }

    if (unifModelInvTr != -1) {
        glm::mat4 modelinvtr = glm::inverse(glm::transpose(model));
        // Pass a 4x4 matrix into a uniform variable in our shader
                        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(unifModelInvTr,
                        // How many matrices to pass
                           1,
                        // Transpose the matrix? OpenGL uses column-major, so no.
                           GL_FALSE,
                        // Pointer to the first element of the matrix
                           &modelinvtr[0][0]);
    }
}

void ShaderProgram::setViewProjMatrix(const glm::mat4 &vp)
{
    // Tell OpenGL to use this shader program for subsequent function calls
    useMe();

    if(unifViewProj != -1) {
    // Pass a 4x4 matrix into a uniform variable in our shader
                    // Handle to the matrix variable on the GPU
    context->glUniformMatrix4fv(unifViewProj,
                    // How many matrices to pass
                       1,
                    // Transpose the matrix? OpenGL uses column-major, so no.
                       GL_FALSE,
                    // Pointer to the first element of the matrix
                       &vp[0][0]);
    }
}

void ShaderProgram::setGeometryColor(glm::vec4 color)
{
    useMe();

    if(unifColor != -1)
    {
        context->glUniform4fv(unifColor, 1, &color[0]);
    }
}

//////This function, as its name implies, uses the passed in GL widget
void ShaderProgram::draw(Drawable &d)
{
    useMe();

    if(d.elemCount() < 0) {
        throw std::out_of_range("Attempting to draw a drawable with m_count of " + std::to_string(d.elemCount()) + "!");
    }

    // Each of the following blocks checks that:
    //   * This shader has this attribute, and
    //   * This Drawable has a vertex buffer for this attribute.
    // If so, it binds the appropriate buffers to each attribute.

    // Remember, by calling bindPos(), we call
    // glBindBuffer on the Drawable's VBO for vertex position,
    // meaning that glVertexAttribPointer associates vs_Pos
    // (referred to by attrPos) with that VBO
    if (attrPos != -1 && d.bindPos()) {
        context->glEnableVertexAttribArray(attrPos);
        context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 0, NULL);
        //                             ^which handle we're referring to
        //                                      ^num of floats for this attr
        //                                                          ^size of stride (in terms of sizeof(float))
        //                                                             ^(void*)(offset)
        // (pos,nor,col)(pos,nor,col)
        // stride = 12*sizeof(float)
        // offset = x*sizeof(float)


    }

    // attrPos := handle to the position attribute (referring to in variables)
    // attrPos == -1, when it does not exist inthe shader prog



    if (attrNor != -1 && d.bindNor()) {
        context->glEnableVertexAttribArray(attrNor);
        context->glVertexAttribPointer(attrNor, 4, GL_FLOAT, false, 0, NULL);
    }

    if (attrCol != -1 && d.bindCol()) {
        context->glEnableVertexAttribArray(attrCol);
        context->glVertexAttribPointer(attrCol, 4, GL_FLOAT, false, 0, NULL);
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindIdx();
    context->glDrawElements(d.drawMode(), d.elemCount(), GL_UNSIGNED_INT, 0);

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrNor != -1) context->glDisableVertexAttribArray(attrNor);
    if (attrCol != -1) context->glDisableVertexAttribArray(attrCol);

    context->printGLErrorLog();
}

//use this draw function for interleaved vbo data for chunks (Elaine 1st)
void ShaderProgram::drawInterleaved(Drawable &d, int textureSlot = 0, int version = 0, int t = 0) {
    useMe();

    if(d.elemCountOpq() < 0) {
        throw std::out_of_range("Attempting to draw a drawable with m_count_opq of " + std::to_string(d.elemCountOpq()) + "!");
    }
    if(d.elemCountTran() < 0) {
        throw std::out_of_range("Attempting to draw a drawable with m_count_opq of " + std::to_string(d.elemCountTran()) + "!");
    }

    if(unifSampler2D != -1)
    {
        context->glUniform1i(unifSampler2D, /*GL_TEXTURE*/textureSlot);
    }
    if (unifTime != -1) {
        context->glUniform1i(unifTime, t);
    }

    if (version == 0) {
        if (attrPos != -1 && attrUv != -1 && attrNor != -1 && animate != -1 && d.bindAllOpaque()) {
            context->glEnableVertexAttribArray(attrPos);
            context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 11 * sizeof(float), (void*)0);
            context->glEnableVertexAttribArray(attrNor);
            context->glVertexAttribPointer(attrNor, 4, GL_FLOAT, false, 11 * sizeof(float), (void*)(4*sizeof(float)));
            context->glEnableVertexAttribArray(attrUv);
            context->glVertexAttribPointer(attrUv, 4, GL_FLOAT, false, 11 * sizeof(float), (void*)(8*sizeof(float)));
            context->glEnableVertexAttribArray(animate);
            context->glVertexAttribPointer(animate, 4, GL_FLOAT, false, 11 * sizeof(float), (void*)(10*sizeof(float)));
        }
        d.bindIdxOpq();
        context->glDrawElements(d.drawMode(), d.elemCountOpq(), GL_UNSIGNED_INT, 0);

        if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
        if (attrNor != -1) context->glDisableVertexAttribArray(attrNor);
        if (attrUv != -1) context->glDisableVertexAttribArray(attrUv);
        if (animate != -1) context->glDisableVertexAttribArray(animate);
        context->printGLErrorLog();

    } else {
        if (attrPos != -1 && attrUv != -1 && attrNor != -1 && animate != -1 && d.bindAllTransparent()) {
            context->glEnableVertexAttribArray(attrPos);
            context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 11 * sizeof(float), (void*)0);
            context->glEnableVertexAttribArray(attrNor);
            context->glVertexAttribPointer(attrNor, 4, GL_FLOAT, false, 11 * sizeof(float), (void*)(4*sizeof(float)));
            context->glEnableVertexAttribArray(attrUv);
            context->glVertexAttribPointer(attrUv, 4, GL_FLOAT, false, 11 * sizeof(float), (void*)(8*sizeof(float)));
            context->glEnableVertexAttribArray(animate);
            context->glVertexAttribPointer(animate, 4, GL_FLOAT, false, 11 * sizeof(float), (void*)(10*sizeof(float)));
        }
        d.bindIdxTran();
        context->glDrawElements(d.drawMode(), d.elemCountTran(), GL_UNSIGNED_INT, 0);

        if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
        if (attrNor != -1) context->glDisableVertexAttribArray(attrNor);
        if (attrUv != -1) context->glDisableVertexAttribArray(attrUv);
        if (animate != -1) context->glDisableVertexAttribArray(animate);
        context->printGLErrorLog();
    }

}


char* ShaderProgram::textFileRead(const char* fileName) {
    char* text;

    if (fileName != NULL) {
        FILE *file = fopen(fileName, "rt");

        if (file != NULL) {
            fseek(file, 0, SEEK_END);
            int count = ftell(file);
            rewind(file);

            if (count > 0) {
                text = (char*)malloc(sizeof(char) * (count + 1));
                count = fread(text, sizeof(char), count, file);
                text[count] = '\0';	//cap off the string with a terminal symbol, fixed by Cory
            }
            fclose(file);
        }
    }
    return text;
}

QString ShaderProgram::qTextFileRead(const char *fileName)
{
    QString text;
    QFile file(fileName);
    if(file.open(QFile::ReadOnly))
    {
        QTextStream in(&file);
        text = in.readAll();
        text.append('\0');
    }
    return text;
}

void ShaderProgram::printShaderInfoLog(int shader)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    context->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0)
    {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        context->glGetShaderInfoLog(shader,infoLogLen, &charsWritten, infoLog);
        qDebug() << "ShaderInfoLog:" << endl << infoLog << endl;
        delete [] infoLog;
    }

    // should additionally check for OpenGL errors here
}

void ShaderProgram::printLinkInfoLog(int prog)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    context->glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0) {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        context->glGetProgramInfoLog(prog, infoLogLen, &charsWritten, infoLog);
        qDebug() << "LinkInfoLog:" << endl << infoLog << endl;
        delete [] infoLog;
    }
}
