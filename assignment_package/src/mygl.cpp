#include "mygl.h"
#include "src/glm_includes.h"
#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <qdatetime.h>
#include "src/blocktypeworker.h"
#include "src/vboworker.h"
#include <QThreadPool>
#include <thread>


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_worldAxes(this),
      m_progLambert(this), m_progFlat(this),
      m_terrain(this), m_player(glm::vec3(48.f, 160.f, 48.f), m_terrain),
      m_currMSecSinceEpoch(QDateTime::currentMSecsSinceEpoch()),
     mp_progSky(this), mp_geomQuad(this),
     isChunksCreated(false),
     m_texture(this),  m_time(0.f), mp_NPC(new NPC(m_terrain, this))
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible


    mp_NPC->generatePosition();
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter() {
//    setMouseTracking(true);
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    //glPointSize(5);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instance of the world axes
    m_worldAxes.create();


    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");


    mp_progSky.create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");
    mp_geomQuad.create();

    // Set a color with which to draw geometry.
    // This will ultimately not be used when you change
    // your program to render Chunks with vertex colors
    // and UV coordinates
    m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    //transparency (Elaine2)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_texture.create(":/textures/minecraft_textures_all.png");
    m_texture.load(0);

}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);
    mp_progSky.setViewProjMatrix(glm::inverse(viewproj));

    mp_progSky.useMe();
    this->glUniform2i(mp_progSky.unifDimensions, width() * this->devicePixelRatio(), height() * this->devicePixelRatio());
    this->glUniform3f(mp_progSky.unifEye, m_player.mcr_camera.mcr_position.x, m_player.mcr_camera.mcr_position.y, m_player.mcr_camera.mcr_position.z);


    printGLErrorLog();

}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    // Calculate dT
    float dT = (QDateTime::currentMSecsSinceEpoch() - m_currMSecSinceEpoch) / 1000.0f;
    m_currMSecSinceEpoch = QDateTime::currentMSecsSinceEpoch();

    m_player.tick(dT, m_inputs);

    // For every terrain generation zone in this radius that does not yet exist in Terrain's m_generatedTerrain,
    // spawn a thread to fill that zone's Chunks with procedural height field BlockType data.
    // Check if new Terrain Zene Chunks need to be crasted an populated
    std::vector<int64_t> terrainNotExpanded = m_terrain.checkExpansion(m_player.getPosition());

    // expected :: 24

    // Spawn worker threads to populate BlockType data in new Chunks
    std::vector<std::vector<Chunk*>> terrainsChunk;
    for (unsigned int i = 0; i < terrainNotExpanded.size(); i++) {
        std::vector<Chunk*> localChunks;
        for (int x = 0; x < 4; x++) {
            for (int z = 0; z < 4; z++) {
                glm:: ivec2 tz_coords = toCoords(terrainNotExpanded.at(i));
                Chunk* c = m_terrain.createChunkAt(tz_coords.x + 16 * x, tz_coords.y + 16 * z);
                localChunks.push_back(c);
            }
        }
        terrainsChunk.push_back(localChunks);
    }

    for (unsigned int i = 0; i < terrainNotExpanded.size(); i++) {
        BlockTypeWorker *bWorker = new BlockTypeWorker(&m_terrain, terrainNotExpanded.at(i), terrainsChunk[i], &m_terrain.chunksWithOnlyBlockData, &m_terrain.mutexWithOnlyBlockData);
        QThreadPool::globalInstance()->start(bWorker);
    }

    m_terrain.mutexWithOnlyBlockData.lock();
    for (Chunk *c : m_terrain.chunksWithOnlyBlockData) {
        VBOWorker *vboWorker = new VBOWorker(&m_terrain,
                                             &m_terrain.chunksWithVBOData,
                                             c,
                                             &m_terrain.mutexChunksWithVBOData);
        QThreadPool::globalInstance()->start(vboWorker);
    }
    m_terrain.chunksWithOnlyBlockData.clear();
    m_terrain.mutexWithOnlyBlockData.unlock();

    m_terrain.mutexChunksWithVBOData.lock();
    for (ChunkVBOData c: m_terrain.chunksWithVBOData) {
        c.associated_chunk->sendToGPU(&c.vertex_opq_data, &c.idx_opq_data,
                                      &c.vertex_tran_data, &c.idx_tran_data);
    }
    m_terrain.chunksWithVBOData.clear();
    m_terrain.mutexChunksWithVBOData.unlock();

    isChunksCreated = true;

    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setModelMatrix(glm::mat4());

    //elaine2
    m_texture.bind(0);
    m_terrain.setTime(m_time);
    m_time++;

    //elaine3 day and night
    mp_progSky.setViewProjMatrix(glm::inverse(m_player.mcr_camera.getViewProj()));
    mp_progSky.useMe();
    this->glUniform3f(mp_progSky.unifEye, m_player.mcr_camera.mcr_position.x, m_player.mcr_camera.mcr_position.y, m_player.mcr_camera.mcr_position.z);
    this->glUniform1f(mp_progSky.unifTime, m_time);
    mp_progSky.draw(mp_geomQuad);

    renderTerrain();

    mp_NPC->destroy();

    glDisable(GL_DEPTH_TEST);
    m_progFlat.setModelMatrix(glm::mat4());
    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    glEnable(GL_DEPTH_TEST);
}

// TODO: Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info) (Elaine 1st)
void MyGL::renderTerrain() {

    int currX = glm::floor(m_player.mcr_position.x / 64.f) * 64;
    int minX = currX - 128;
    int maxX = currX + 128;
    int currZ = glm::floor(m_player.mcr_position.z / 64.f) * 64;
    int minZ = currZ - 128;
    int maxZ = currZ + 128;
    //depending on the player's position, render terrain including a new chunk

   m_terrain.draw(minX, maxX, minZ, maxZ, &m_progLambert);
}

// construct an inputbundle in keypress event with appropriate info
// and read the info to update the velocity and position
void MyGL::keyPressEvent(QKeyEvent *e) {

    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    }
    if (!e->isAutoRepeat()) {
        keyPressUpdate(e);
    }
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    if (!e->isAutoRepeat()) {
        keyReleaseUpdate(e);
    }

}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    // For mac
    float dx = (e->pos().x() - this->width() * 0.5 + this->pos().x()) / (float) width();
    float dy = (e->pos().y() - this->height() * 0.5 + this->pos().y()) / (float) height();
    m_player.rotateOnUpGlobal(-dx * 360 * 0.005f);
    m_player.rotateOnRightLocal(-dy * 360 * 0.005f);

    moveMouseToCenter();
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_player.destroyBlock(&m_terrain);
    } else if (e->button() == Qt::RightButton) {
        m_player.createBlock(&m_terrain);
    }
}

void MyGL::keyPressUpdate(QKeyEvent *e) {
    if (m_inputs.isFlightMode) {
        m_player.flying->setLoops(QSound::Infinite);
        m_player.flying->play();
        if (e->key() == Qt::Key_W) {
            m_inputs.wPressed = true;
        } else if (e->key() == Qt::Key_S) {
            m_inputs.sPressed = true;
        } else if (e->key() == Qt::Key_D) {
            m_inputs.dPressed = true;
        } else if (e->key() == Qt::Key_A) {
            m_inputs.aPressed = true;
        } else if (e->key() == Qt::Key_Q) {
            m_inputs.qPressed = true;
        } else if (e->key() == Qt::Key_E) {
            m_inputs.ePressed = true;
        } else if (e->key() == Qt::Key_F) {
            m_inputs.isFlightMode = false;
        }
    } else {
        m_player.flying->stop();
        if (m_inputs.isInWater) {
            m_player.footstep->stop();
            m_player.footstep = new QSound(":/sound/swimming.wav");
        } else {
            m_player.footstep->stop();
            m_player.footstep = new QSound(":/sound/footsteps-grass.wav");
        }
        if (e->key() == Qt::Key_W) {
            m_player.footstep->play();
            m_inputs.wPressed = true;
        } else if (e->key() == Qt::Key_S) {
            m_player.footstep->play();
            m_inputs.sPressed = true;
        } else if (e->key() == Qt::Key_D) {
            m_player.footstep->play();
            m_inputs.dPressed = true;
        } else if (e->key() == Qt::Key_A) {
            m_player.footstep->play();
            m_inputs.aPressed = true;
        } else if (e->key() == Qt::Key_Space) {
            m_player.jump->play();
            m_inputs.spacePressed = true;
        } else if (e->key() == Qt::Key_F) {
            m_inputs.isFlightMode = true;
        }
    }
}

void MyGL::keyReleaseUpdate(QKeyEvent *e) {
    m_player.footstep->stop();
    m_player.jump->stop();
    if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = false;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = false;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = false;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = false;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = false;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = false;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = false;
    }
}
