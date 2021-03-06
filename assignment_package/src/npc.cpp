#include "npc.h"

NPC::NPC(const Terrain &terrain, OpenGLContext *context)
    : Drawable(context), mcr_terrain(terrain), ifAxis(-1), isOnGround(false), isCollision(false)
{
}

void NPC::tick(float dT) {
    // update Physics
    updatePhysicsInfo();
    processMovement(dT);
    // draw them
    create();
}

void NPC::updatePhysicsInfo() {
    // decide the new velocity randomly if a collision happens
    // the npc should be able to randomly jump/change direction
    // to be called from
    int action;
    if (!isOnGround) {
        m_velocity = glm::vec3(0.f, -30.f, 0.f);
    }

    if (isCollision) {
        //TODO : make the NPC jump
        m_velocity += glm::vec3(0.f, 10.f, 0.f);

    } else {
        // TODO : randomly switch direction
        // try to prevent the npc from switching the direciton too often
        action = setRandomMovement();
        assigneDirection(action);
    }
}

void NPC::processMovement(float dT) {
    glm::vec3 rayDirection = m_velocity * dT;
    detectCollision(&rayDirection, mcr_terrain);
    m_position += rayDirection;
}

void NPC::assigneDirection(int direction) {
    // set velocity
    switch(direction) {
    case NORTH:
        m_velocity = glm::vec3(0.f, 0.f, 10.f);
        break;
    case WEST:
        m_velocity = glm::vec3(-10.f, 0.f, 0.f);
        break;
    case SOUTH:
        m_velocity = glm::vec3(0.f, 0.f, -10.f);
        break;
    case EAST:
        m_velocity = glm::vec3(10.f, 0.f, 0.f);
    }
}

void NPC::detectCollision(glm::vec3 *rayDirection, const Terrain &terrain) {
      glm::vec3 bottomLeftVertex = this->m_position - glm::vec3(0.5f, 0.f, 0.5f);
      glm::ivec3 out_blockHit = glm::ivec3();
      float out_dist = 0.f;

      for (int x = 0; x <= 1; x++) {
          for (int z = 0; z >= -1; z--) {
              for (int y = 0; y <= 2; y++) {
                  glm::vec3 rayOrigin = bottomLeftVertex + glm::vec3(x, y, z);
                  for (int i = 0; i < 3; i++) {

                  }
                  if (gridMarch(rayOrigin, *rayDirection, terrain, &out_dist, &out_blockHit)) {
                      float distance = glm::min(out_dist - 0.005f, glm::abs(glm::length(this->m_position - glm::vec3(out_blockHit))));
                      *rayDirection = distance * glm::normalize(*rayDirection);
                  }
              }
          }
      }
}

bool NPC::gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    float maxLen = glm::length(rayDirection); // farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // now all t values represent world dist

    float curr_t = 0.f;
    while (curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for (int i = 0; i < 3; ++i) { // Iterate over the three axes
            if (rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i]));

                // If the player is "exactly on an interface then
                // they'll never move if they're looking in a negative direction
                if (currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bound errors
                if (axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if (interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        ifAxis = interfaceAxis;
        curr_t += min_t;
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0, 0, 0);
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If the currCell contains something other than empty, return curr_t
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);

        if (cellType != EMPTY) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            isCollision = true;
            return true;
        }
    }
    isCollision = false;
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}


bool NPC::isOnGroundLevel(const Terrain &terrain) {

    glm::vec3 bottomLeftVertex = this->m_position - glm::vec3(0.5f, 0.f, 0.5f);
    for (int x = 0; x <= 1; x++) {
        for (int z = 0; z <= 1; z++) {
            if (terrain.getBlockAt(floor(bottomLeftVertex[0]) + x,
                                   floor(bottomLeftVertex[1] - 0.005f),
                                   floor(bottomLeftVertex[2]) + z) != EMPTY) {

                isOnGround = true;
                m_velocity.y = 0.f;
                return true;
            }
        }
    }
    isOnGround = false;
    return false;
}

void NPC::generatePosition() {
    m_position = glm::vec3(64.f, 140.f, 64.f);
}

void NPC::create() {
    std::vector<GLuint> idx;
    std::vector<float> pos_nor_uv_anim;

    std::vector<glm::vec4> pos;
    std::vector<glm::vec4> nor;
    std::vector<glm::vec2> uv;
    std::vector<float> anim;
    //createVBO(&idx, &pos_nor_uv);
    createTestVBO(&idx, &pos, &nor, &uv, &anim);
    interleaveVBO(&pos, &nor, &uv, &anim, &pos_nor_uv_anim);
    sendVBOToGPU(&idx, &pos_nor_uv_anim);
}

void NPC::createTestVBO(std::vector<GLuint> *idx,
                        std::vector<glm::vec4> *pos,
                        std::vector<glm::vec4> *nor,
                        std::vector<glm::vec2> *uv,
                        std::vector<float> *anim) {
     //simple case
    idx->clear();
    for (int i = 0; i < 6; i++) {
        idx->push_back(i * 4);
        idx->push_back(i * 4 + 1);
        idx->push_back(i * 4 + 2);
        idx->push_back(i * 4);
        idx->push_back(i * 4 + 2);
        idx->push_back(i * 4 + 3);
    }
    m_count_npc_opq = idx->size();

    pos->clear();
    nor->clear();
    uv->clear();
    anim->clear();

    glm::vec2 tex = glm::vec2(1.0f / 16.0f * 5, 1.0f / 16.0f * 13);


    // bottom
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y, m_position.z + 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y, m_position.z + 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y, m_position.z - 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y, m_position.z - 0.5f, 1));
    nor->push_back(glm::vec4(0, -1, 0, 0));
    nor->push_back(glm::vec4(0, -1, 0, 0));
    nor->push_back(glm::vec4(0, -1, 0, 0));
    nor->push_back(glm::vec4(0, -1, 0, 0));
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);

    // up
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.0f, m_position.z + 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.0f, m_position.z - 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.0f, m_position.z - 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.0f, m_position.z + 0.5f, 1));
    nor->push_back(glm::vec4(0, 1, 0, 0));
    nor->push_back(glm::vec4(0, 1, 0, 0));
    nor->push_back(glm::vec4(0, 1, 0, 0));
    nor->push_back(glm::vec4(0, 1, 0, 0));
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);

    //right
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.0f, m_position.z + 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y, m_position.z + 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y, m_position.z - 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.0f, m_position.z - 0.5f, 1));
    nor->push_back(glm::vec4(1, 0, 0, 0));
    nor->push_back(glm::vec4(1, 0, 0, 0));
    nor->push_back(glm::vec4(1, 0, 0, 0));
    nor->push_back(glm::vec4(1, 0, 0, 0));
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);


    //left
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y , m_position.z - 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y, m_position.z + 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.0f, m_position.z + 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.0f, m_position.z - 0.5f, 1));
    nor->push_back(glm::vec4(-1, 0, 0, 0));
    nor->push_back(glm::vec4(-1, 0, 0, 0));
    nor->push_back(glm::vec4(-1, 0, 0, 0));
    nor->push_back(glm::vec4(-1, 0, 0, 0));
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);

    // forward
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y, m_position.z + 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y, m_position.z + 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.0f, m_position.z + 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.0f, m_position.z + 0.5f, 1));
    nor->push_back(glm::vec4(0, 0, 1, 0));
    nor->push_back(glm::vec4(0, 0, 1, 0));
    nor->push_back(glm::vec4(0, 0, 1, 0));
    nor->push_back(glm::vec4(0, 0, 1, 0));
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);


    // back
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y, m_position.z - 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.f, m_position.z - 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.0f, m_position.z - 0.5f, 1));
    pos->push_back(glm::vec4(m_position.x + 0.5f, m_position.y, m_position.z - 0.5f, 1));
    nor->push_back(glm::vec4(0, 0, -1, 0));
    nor->push_back(glm::vec4(0, 0, -1, 0));
    nor->push_back(glm::vec4(0, 0, -1, 0));
    nor->push_back(glm::vec4(0, 0, -1, 0));
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    uv->push_back(tex);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);
    anim->push_back(0);
}

void NPC::interleaveVBO(std::vector<glm::vec4> *pos,
                        std::vector<glm::vec4> *nor,
                        std::vector<glm::vec2> *uv,
                        std::vector<float> *anim,
                        std::vector<float> *allOpq) {

    for (unsigned int i = 0; i < pos->size(); ++i) {
        allOpq->push_back(pos->at(i)[0]);
        allOpq->push_back(pos->at(i)[1]);
        allOpq->push_back(pos->at(i)[2]);
        allOpq->push_back(pos->at(i)[3]);
        allOpq->push_back(nor->at(i)[0]);
        allOpq->push_back(nor->at(i)[1]);
        allOpq->push_back(nor->at(i)[2]);
        allOpq->push_back(nor->at(i)[3]);
        allOpq->push_back(uv->at(i)[0]);
        allOpq->push_back(uv->at(i)[1]);
        allOpq->push_back(anim->at(i));
    }
}
void NPC::createVBO(std::vector<GLuint> *idx, std::vector<glm::vec4> *pos_nor_uv) {

    for(int i = 0; i < 36; i++){
        idx->push_back(i*4);
        idx->push_back(i*4+1);
        idx->push_back(i*4+2);
        idx->push_back(i*4);
        idx->push_back(i*4+2);
        idx->push_back(i*4+3);
    }
    m_count = idx->size();
    glm::vec2 tex = glm::vec2(1.0f / 16.0f * 5, 1.0f / 16.0f * 13);
    float cos = 2.0f;

    pos_nor_uv->clear();

    /// add the body vertices
    // add front
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y + 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y + 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;
    // add back
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y + 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y + 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;
    // add left
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y + 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y + 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;
    // add right
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y + 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y + 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;
    // add top
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y + 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y + 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y + 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y + 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;
    // add bottom
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;


    /// add head vertices
    // add front
    glm::vec2 faceTexture = glm::vec2(57, 197) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(faceTexture.x, faceTexture.y, cos, 0));
    faceTexture = glm::vec2(57, 146) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 0.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(faceTexture.x, faceTexture.y, cos, 0));
    faceTexture = glm::vec2(108, 146) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 0.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(faceTexture.x, faceTexture.y, cos, 0));
    faceTexture = glm::vec2(108, 197) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(faceTexture.x, faceTexture.y, cos, 0));
    // add back
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 0.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 0.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;
    // add left
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 0.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 0.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;
    // add right
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 0.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 0.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;
    // add top
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 1.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 1.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;
    // add bottom
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 0.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y -= 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.5f, m_position.y + 0.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 0.25f, m_position.z + 1.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.y += 1.0f / 16.0f;
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.5f, m_position.y + 0.25f, m_position.z + 0.5f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(tex.x, tex.y, cos, 0));
    tex.x -= 1.0f / 16.0f;

    /// add back left leg vertices
    glm::vec2 legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    // add front
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add back
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add left
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add right
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add top
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add bottom
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));

    /// add back right leg vertices
    // add front
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add back
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add right
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add left
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z - 0.1f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z - 0.1f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add top
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add bottom
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z - 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z - 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));

    /// add front left leg vertices
    // add front
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add back
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add left
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add right
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add top
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add bottom
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.75f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x - 0.35f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));

    /// add front right leg vertices
    // add front
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, -1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add back
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 0, 1, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add right
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add left
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(-1, 0, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add top
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 0.5f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, 1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    // add bottom
    legTexture = glm::vec2(30, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(30, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.75f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 11) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z + 1.0f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
    legTexture = glm::vec2(58, 39) * (1 / 256.0f);
    pos_nor_uv->push_back(glm::vec4(m_position.x + 0.35f, m_position.y - 1.0f, m_position.z + 0.6f, 1));
    pos_nor_uv->push_back(glm::vec4(0, -1, 0, 0));
    pos_nor_uv->push_back(glm::vec4(legTexture.x, legTexture.y, cos, 0));
}

void NPC::sendVBOToGPU(std::vector<GLuint> *idx, std::vector<float> *pos_nor_uv_anim){
    generateNPCIdxOpq();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufNPCIdxOpq);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx->size() * sizeof(GLuint), idx->data(), GL_STATIC_DRAW);

    generateNPCAllOpq();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufNPCAllOpq);
    mp_context->glBufferData(GL_ARRAY_BUFFER, pos_nor_uv_anim->size() * sizeof(float), pos_nor_uv_anim->data(), GL_STATIC_DRAW);
}

int NPC::setRandomMovement() {
    return rand() * 4;
}
