#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GLUT/glut.h>
#include <cmath>
#include <cstdlib>
#include <vector>

// dimensiunile ferestrei
static int width = 800;
static int height = 600;

float camX = 0.0f, camY = 10.0f, camZ = 40.0f;
float camYaw = -90.0f;
float camPitch = -15.0f;
float camSpeed = 0.5f;

bool isDragging = false;
int lastMouseX = 400, lastMouseY = 300;
float mouseSensitivity = 0.2f;

bool keys[256] = {false};

// variabile masina
float masinaX = 25.0f, masinaZ = 0.0f;
float masinaYaw = 90.0f;
float masinaSpeed = 0.0f;
float masinaMaxSpeed = 0.8f;
bool keySpecial[256] = { false };

struct Obstacol {
    float x, z;
    float latime, lungime;
};

std::vector<Obstacol> obstacole;

void adaugaObstacol(float x, float z, float w, float l) {
    Obstacol obs = { x, z, w, l };
    obstacole.push_back(obs);
}

GLuint texIarba, texOrizont, texPiatra, texAsfalt, texFrunze, texTrunk;

// pozitia luminii soarelui
float globalLightPos[] = { 30.0f, 80.0f, 30.0f, 0.0f };

struct Copac {
    float x, z, scara;
};

#define MAX_COPACI 200
Copac copaci[MAX_COPACI];
int nrCopaci = 0;

struct Stalp {
    float x, z;
};
Stalp stalpi[] = {
    { 28.0f, 0.0f },
    { -28.0f, 0.0f },
    { 0.0f, 22.0f }
};
int nrStalpi = 3;

// incarcarea texturilor
GLuint LoadTexture(const char* filename) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    stbi_set_flip_vertically_on_load(true);
    int w, h, nrChannels;
    unsigned char* data = stbi_load(filename, &w, &h, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    }
    stbi_image_free(data);
    return texture;
}

// denivelarile terenului
float calculInaltime(float x, float z) {
    float y = 0.0f;
    y += 1.5f * sin(x * 0.07f + 1.3f) * cos(z * 0.09f + 0.7f);
    y += 0.8f * cos(x * 0.13f - z * 0.11f + 2.1f);
    y += 0.4f * sin(x * 0.21f + z * 0.17f - 0.5f);

    float mx = x + 40.0f; float mz = z + 40.0f;
    float dm = sqrt(mx * mx + mz * mz);
    float unghi = atan2(mz, mx);
    float raza = 14.0f + 3.0f * sin(unghi * 3.0f) + 2.0f * cos(unghi * 5.0f + 1.0f);

    if (dm < raza) {
        float baza = 12.0f * cos((dm / raza) * 1.57f);
        float zgomot = 2.0f * sin(x * 0.9f + 3.7f) * cos(z * 1.1f + 1.2f)
            + 1.2f * sin(x * 1.7f - z * 1.3f + 5.1f)
            + 0.7f * cos(x * 2.3f + z * 1.9f - 2.8f);
        float factor = 1.0f - (dm / raza);
        y += baza + zgomot * factor;
    }
    return y;
}

// tranzitia iarba-munte
float calcAlphaMunte(float x, float z) {
    float mx = x + 40.0f; float mz = z + 40.0f;
    float dm = sqrt(mx * mx + mz * mz);
    float unghi = atan2(mz, mx);
    float raza = 14.0f + 3.0f * sin(unghi * 3.0f) + 2.0f * cos(unghi * 5.0f + 1.0f);
    if (dm >= raza) return 0.0f;
    float t = dm / raza;
    if (t < 0.5f) return 1.0f;
    return 1.0f - (t - 0.5f) / 0.5f;
}

void getNormalaTeren(float x, float z, float normal[3]) {
    float pas = 0.1f;
    float hL = calculInaltime(x - pas, z);
    float hR = calculInaltime(x + pas, z);
    float hD = calculInaltime(x, z - pas);
    float hU = calculInaltime(x, z + pas);
    normal[0] = hL - hR;
    normal[1] = 2.0f * pas;
    normal[2] = hD - hU;
    float lungime = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    normal[0] /= lungime; normal[1] /= lungime; normal[2] /= lungime;
}

void aplicaMatriceUmbra(float punctSol[3], float normala[3], float lumina[4]) {
    float d = -(normala[0] * punctSol[0] + normala[1] * punctSol[1] + normala[2] * punctSol[2]);
    float dot = normala[0] * lumina[0] + normala[1] * lumina[1] + normala[2] * lumina[2] + d * lumina[3];
    float shadowMat[16];

    shadowMat[0] = dot - lumina[0] * normala[0]; shadowMat[4] = -lumina[0] * normala[1]; shadowMat[8] = -lumina[0] * normala[2]; shadowMat[12] = -lumina[0] * d;
    shadowMat[1] = -lumina[1] * normala[0]; shadowMat[5] = dot - lumina[1] * normala[1]; shadowMat[9] = -lumina[1] * normala[2]; shadowMat[13] = -lumina[1] * d;
    shadowMat[2] = -lumina[2] * normala[0]; shadowMat[6] = -lumina[2] * normala[1]; shadowMat[10] = dot - lumina[2] * normala[2]; shadowMat[14] = -lumina[2] * d;
    shadowMat[3] = -lumina[3] * normala[0]; shadowMat[7] = -lumina[3] * normala[1]; shadowMat[11] = -lumina[3] * normala[2]; shadowMat[15] = dot - lumina[3] * d;

    glMultMatrixf(shadowMat);
}

bool verificaColiziune(float viitorX, float viitorZ) {
    float masinaW = 2.8f;
    float masinaL = 2.8f;

    for (const auto& obs : obstacole) {
        if (viitorX - masinaW/2 < obs.x + obs.latime/2 &&
            viitorX + masinaW/2 > obs.x - obs.latime/2 &&
            viitorZ - masinaL/2 < obs.z + obs.lungime/2 &&
            viitorZ + masinaL/2 > obs.z - obs.lungime/2)
        {
            return true;
        }
    }
    return false;
}

// deseneaza iarba pe teren
void drawTeren() {
    glBindTexture(GL_TEXTURE_2D, texIarba);
    glColor3f(1.0f, 1.0f, 1.0f);

    float dim = 50.0f, pas = 1.0f;
    glBegin(GL_QUADS);
    for (float z = -dim; z < dim; z += pas) {
        for (float x = -dim; x < dim; x += pas) {
            float y1 = calculInaltime(x, z); float y2 = calculInaltime(x + pas, z);
            float y3 = calculInaltime(x + pas, z + pas); float y4 = calculInaltime(x, z + pas);
            float u1 = (x + dim) / 5.0f, v1 = (z + dim) / 5.0f;
            float u2 = (x + pas + dim) / 5.0f, v2 = (z + pas + dim) / 5.0f;
            float n[3];

            getNormalaTeren(x, z + pas, n); glNormal3f(n[0], n[1], n[2]);
            glTexCoord2f(u1, v2); glVertex3f(x, y4, z + pas);
            getNormalaTeren(x + pas, z + pas, n); glNormal3f(n[0], n[1], n[2]);
            glTexCoord2f(u2, v2); glVertex3f(x + pas, y3, z + pas);
            getNormalaTeren(x + pas, z, n); glNormal3f(n[0], n[1], n[2]);
            glTexCoord2f(u2, v1); glVertex3f(x + pas, y2, z);
            getNormalaTeren(x, z, n); glNormal3f(n[0], n[1], n[2]);
            glTexCoord2f(u1, v1); glVertex3f(x, y1, z);
        }
    }
    glEnd();
}

// deseneaza textura de munte peste teren
void drawMunte() {
    glBindTexture(GL_TEXTURE_2D, texPiatra);
    float centruX = -40.0f, centruZ = -40.0f;
    float razaMax = 19.0f, pas = 1.0f, dim = 50.0f;

    glBegin(GL_QUADS);
    for (float z = centruZ - razaMax; z < centruZ + razaMax; z += pas) {
        for (float x = centruX - razaMax; x < centruX + razaMax; x += pas) {
            float a1 = calcAlphaMunte(x, z); float a2 = calcAlphaMunte(x + pas, z);
            float a3 = calcAlphaMunte(x + pas, z + pas); float a4 = calcAlphaMunte(x, z + pas);
            if (a1 == 0.0f && a2 == 0.0f && a3 == 0.0f && a4 == 0.0f) continue;

            float y1 = calculInaltime(x, z); float y2 = calculInaltime(x + pas, z);
            float y3 = calculInaltime(x + pas, z + pas); float y4 = calculInaltime(x, z + pas);
            float u1 = (x + dim) / 3.0f, v1 = (z + dim) / 3.0f;
            float u2 = (x + pas + dim) / 3.0f, v2 = (z + pas + dim) / 3.0f;
            float n[3];

            getNormalaTeren(x, z + pas, n); glNormal3f(n[0], n[1], n[2]);
            glColor4f(1.0f, 1.0f, 1.0f, a4); glTexCoord2f(u1, v2); glVertex3f(x, y4, z + pas);
            getNormalaTeren(x + pas, z + pas, n); glNormal3f(n[0], n[1], n[2]);
            glColor4f(1.0f, 1.0f, 1.0f, a3); glTexCoord2f(u2, v2); glVertex3f(x + pas, y3, z + pas);
            getNormalaTeren(x + pas, z, n); glNormal3f(n[0], n[1], n[2]);
            glColor4f(1.0f, 1.0f, 1.0f, a2); glTexCoord2f(u2, v1); glVertex3f(x + pas, y2, z);
            getNormalaTeren(x, z, n); glNormal3f(n[0], n[1], n[2]);
            glColor4f(1.0f, 1.0f, 1.0f, a1); glTexCoord2f(u1, v1); glVertex3f(x, y1, z);
        }
    }
    glEnd();
}

// deseneaza circuitul stradal
void drawDrum() {
    glBindTexture(GL_TEXTURE_2D, texAsfalt);
    glColor3f(1.0f, 1.0f, 1.0f);
    float a = 25.0f, b = 18.0f, latimeDrum = 4.0f;
    int nrSegmente = 120;
    float vAcum = 0.0f;

    glBegin(GL_QUADS);
    for (int i = 0; i < nrSegmente; i++) {
        float t1 = (float)i / nrSegmente * 2.0f * M_PI;
        float t2 = (float)(i + 1) / nrSegmente * 2.0f * M_PI;

        float cx1 = a * cos(t1), cz1 = b * sin(t1);
        float cx2 = a * cos(t2), cz2 = b * sin(t2);

        float tx1 = -a * sin(t1), tz1 = b * cos(t1);
        float tx2 = -a * sin(t2), tz2 = b * cos(t2);

        float len1 = sqrt(tx1 * tx1 + tz1 * tz1); float nx1 = tz1 / len1, nz1 = -tx1 / len1;
        float len2 = sqrt(tx2 * tx2 + tz2 * tz2); float nx2 = tz2 / len2, nz2 = -tx2 / len2;

        float half = latimeDrum / 2.0f;

        float x1i = cx1 - nx1 * half, z1i = cz1 - nz1 * half; float x1e = cx1 + nx1 * half, z1e = cz1 + nz1 * half;
        float x2i = cx2 - nx2 * half, z2i = cz2 - nz2 * half; float x2e = cx2 + nx2 * half, z2e = cz2 + nz2 * half;

        float y1i = calculInaltime(x1i, z1i) + 0.05f; float y1e = calculInaltime(x1e, z1e) + 0.05f;
        float y2i = calculInaltime(x2i, z2i) + 0.05f; float y2e = calculInaltime(x2e, z2e) + 0.05f;

        float dx = cx2 - cx1, dz = cz2 - cz1;
        float lungSegment = sqrt(dx * dx + dz * dz);
        float vUrm = vAcum + lungSegment / latimeDrum;

        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, vUrm);  glVertex3f(x2i, y2i, z2i);
        glTexCoord2f(1.0f, vUrm);  glVertex3f(x2e, y2e, z2e);
        glTexCoord2f(1.0f, vAcum); glVertex3f(x1e, y1e, z1e);
        glTexCoord2f(0.0f, vAcum); glVertex3f(x1i, y1i, z1i);

        vAcum = vUrm;
    }
    glEnd();
}

// deseneaza bradul
void drawCopac(float x, float z, float scara, bool isShadow = false, float shadowAlpha = 0.4f) {
    float y = calculInaltime(x, z);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scara, scara, scara);

    if (!isShadow) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texTrunk);
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_TEXTURE_2D);
        glColor4f(0.0f, 0.0f, 0.0f, shadowAlpha);
    }

    float rT = 0.2f, hT = 1.8f; int seg = 10;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= seg; i++) {
        float t = (float)i / seg * 2.0f * M_PI;
        float cx = rT * cos(t); float cz = rT * sin(t);
        float u = (float)i / seg * 2.0f;
        glNormal3f(cos(t), 0.0f, sin(t));
        glTexCoord2f(u, 1.0f); glVertex3f(cx, hT, cz);
        glTexCoord2f(u, 0.0f); glVertex3f(cx, 0.0f, cz);
    }
    glEnd();

    if (!isShadow) glBindTexture(GL_TEXTURE_2D, texFrunze);

    int etaje = 5; int segCon = 16;
    float startY = hT - 0.3f;

    for (int e = 0; e < etaje; e++) {
        float et = (float)e / etaje;
        float razaBaza = (e == etaje - 1) ? 0.5f : 2.2f * (1.0f - et * 0.55f);
        float hCon = 2.0f * (1.0f - et * 0.15f);
        float bazaY = startY + e * 1.1f;
        float varfY = bazaY + hCon;
        float varfOfsX = 0.1f * sin(e * 2.3f + scara * 5.0f);
        float varfOfsZ = 0.1f * cos(e * 3.1f + scara * 7.0f);

        if (!isShadow) glColor3f(1.0f, 1.0f, 1.0f);

        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.5f, 0.0f); glVertex3f(varfOfsX, varfY, varfOfsZ);
        for (int i = 0; i <= segCon; i++) {
            float a = (float)i / segCon * 2.0f * M_PI;
            float rVar = razaBaza * (1.0f + 0.15f * sin(a * 3.0f + e * 1.7f) + 0.1f * cos(a * 5.0f + e * 2.3f));
            float yVar = bazaY + 0.15f * sin(a * 4.0f + e * 1.1f);
            glNormal3f(cos(a), 0.5f, sin(a));
            glTexCoord2f((float)i / segCon * 5.0f, 1.0f);
            glVertex3f(rVar * cos(a), yVar, rVar * sin(a));
        }
        glEnd();

        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(0.5f, 0.5f); glVertex3f(0.0f, bazaY, 0.0f);
        for (int i = segCon; i >= 0; i--) {
            float a = (float)i / segCon * 2.0f * M_PI;
            float rVar = razaBaza * (1.0f + 0.15f * sin(a * 3.0f + e * 1.7f) + 0.1f * cos(a * 5.0f + e * 2.3f));
            float yVar = bazaY + 0.15f * sin(a * 4.0f + e * 1.1f);
            glTexCoord2f(0.5f + 0.5f * cos(a), 0.5f + 0.5f * sin(a));
            glVertex3f(rVar * cos(a), yVar, rVar * sin(a));
        }
        glEnd();
    }
    if (!isShadow) glColor3f(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void drawStalp(float x, float z, bool isShadow = false) {
    float ySol = calculInaltime(x, z);
    glPushMatrix();
    glTranslatef(x, ySol, z);

    float inalt = 6.0f; float rad = 0.2f;

    if (!isShadow) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texAsfalt);
        glColor3f(0.4f, 0.4f, 0.4f);
    } else {
        glDisable(GL_TEXTURE_2D);
        glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    }

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 12; i++) {
        float t = (float)i / 12 * 2.0f * M_PI;
        float nx = cos(t), nz = sin(t);
        glNormal3f(nx, 0.0f, nz);
        glTexCoord2f((float)i / 12, 1.0f); glVertex3f(rad * nx, inalt, rad * nz);
        glTexCoord2f((float)i / 12, 0.0f); glVertex3f(rad * nx, 0.0f, rad * nz);
    }
    glEnd();

    if (!isShadow) {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 0.9f, 0.6f);
        glTranslatef(0.0f, inalt + 0.2f, 0.0f);
        glutSolidSphere(0.4f, 12, 12);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
    }
    glPopMatrix();
}

// deseneaza peretii orizontului
void drawSkybox() {
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, texOrizont);
    float d = 50.0f;
    float yJos = -5.0f;
    float ySus = 50.0f;
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-d, yJos, -d); glTexCoord2f(1.0f, 0.0f); glVertex3f( d, yJos, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( d, ySus, -d); glTexCoord2f(0.0f, 1.0f); glVertex3f(-d, ySus, -d);

    glTexCoord2f(1.0f, 0.0f); glVertex3f(d, yJos, -d); glTexCoord2f(0.0f, 0.0f); glVertex3f(d, yJos,  d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(d, ySus,  d); glTexCoord2f(1.0f, 1.0f); glVertex3f(d, ySus, -d);

    glTexCoord2f(0.0f, 0.0f); glVertex3f( d, yJos, d); glTexCoord2f(1.0f, 0.0f); glVertex3f(-d, yJos, d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-d, ySus, d); glTexCoord2f(0.0f, 1.0f); glVertex3f( d, ySus, d);

    glTexCoord2f(1.0f, 0.0f); glVertex3f(-d, yJos,  d); glTexCoord2f(0.0f, 0.0f); glVertex3f(-d, yJos, -d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-d, ySus, -d); glTexCoord2f(1.0f, 1.0f); glVertex3f(-d, ySus,  d);
    glEnd();
    glEnable(GL_LIGHTING);
}

// desenare masina
void drawMasina() {
    float hSol = calculInaltime(masinaX, masinaZ);

    glPushMatrix();
    glTranslatef(masinaX, hSol, masinaZ);
    glRotatef(masinaYaw, 0.0f, -1.0f, 0.0f);

    glDisable(GL_TEXTURE_2D);

    // rotile
    glDisable(GL_LIGHTING);
    glColor3f(0.1f, 0.1f, 0.1f);
    float wheelY = 0.4f;
    float wheelX = 1.2f;
    float wheelZ = 0.8f;
    for(int i = -1; i <= 1; i += 2) {
        for(int j = -1; j <= 1; j += 2) {
            glPushMatrix();
            glTranslatef(i * wheelX, wheelY, j * wheelZ);
            glScalef(0.4f, 0.4f, 0.15f);
            glutSolidSphere(1.0f, 15, 15);
            glPopMatrix();
        }
    }
    glEnable(GL_LIGHTING);

    // sasiul
    glColor3f(0.8f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 0.7f, 0.0f);
    glScalef(3.4f, 0.6f, 1.4f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // cabina
    glDisable(GL_LIGHTING);
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(-0.2f, 1.2f, 0.0f);
    glScalef(1.8f, 0.6f, 1.2f);
    glutSolidCube(1.0f);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // faruri
    glColor3f(1.0f, 1.0f, 0.2f);
    glPushMatrix(); glTranslatef(1.7f, 0.7f,  0.5f); glutSolidSphere(0.15f, 10, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(1.7f, 0.7f, -0.5f); glutSolidSphere(0.15f, 10, 10); glPopMatrix();

    // stopuri
    glColor3f(1.0f, 0.0f, 0.0f);
    glPushMatrix(); glTranslatef(-1.7f, 0.7f,  0.5f); glutSolidSphere(0.15f, 10, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.7f, 0.7f, -0.5f); glutSolidSphere(0.15f, 10, 10); glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

// plasarea brazilor
void genereazaCopaci() {
    srand(42);
    float a = 25.0f, b = 18.0f;
    float centruMX = -40.0f, centruMZ = -40.0f;
    int incercari = 0;

    while (nrCopaci < MAX_COPACI && incercari < 5000) {
        incercari++;
        float x = ((float)rand() / RAND_MAX) * 90.0f - 45.0f;
        float z = ((float)rand() / RAND_MAX) * 90.0f - 45.0f;

        float ex = x / a, ez = z / b;
        if (fabs(sqrt(ex * ex + ez * ez) - 1.0f) < 0.25f) continue;

        float dmx = x - centruMX, dmz = z - centruMZ;
        float distMunte = sqrt(dmx * dmx + dmz * dmz);
        if (distMunte < 22.0f) continue;
        if (distMunte < 35.0f && ((float)rand() / RAND_MAX < 0.7f)) continue;

        bool preAproape = false;
        for (int k = 0; k < nrCopaci; k++) {
            float ddx = copaci[k].x - x, ddz = copaci[k].z - z;
            if (ddx * ddx + ddz * ddz < 9.0f) { preAproape = true; break; }
        }
        if (preAproape) continue;

        copaci[nrCopaci].x = x;
        copaci[nrCopaci].z = z;
        copaci[nrCopaci].scara = 0.5f + ((float)rand() / RAND_MAX) * 0.5f;
        nrCopaci++;
    }
}

// detecteaza cand e apasat mouse-ul
void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) { isDragging = true; lastMouseX = x; lastMouseY = y; }
        else if (state == GLUT_UP) { isDragging = false; }
    }
}

// controlul camerei
void mouseMotionDrag(int x, int y) {
    if (!isDragging) return;
    float dx = (x - lastMouseX) * mouseSensitivity;
    float dy = (lastMouseY - y) * mouseSensitivity;
    lastMouseX = x; lastMouseY = y;
    camYaw += dx; camPitch += dy;
    if (camPitch > 89.0f) camPitch = 89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float yawRad = camYaw * M_PI / 180.0f, pitchRad = camPitch * M_PI / 180.0f;
    float lookX = camX + cos(pitchRad) * cos(yawRad);
    float lookY = camY + sin(pitchRad);
    float lookZ = camZ + cos(pitchRad) * sin(yawRad);
    gluLookAt(camX, camY, camZ, lookX, lookY, lookZ, 0.0, 1.0, 0.0);

    // actualizare pozitii lumini
    glLightfv(GL_LIGHT0, GL_POSITION, globalLightPos);
    for (int s = 0; s < nrStalpi; s++) {
        float pos[] = { stalpi[s].x, calculInaltime(stalpi[s].x, stalpi[s].z) + 6.0f, stalpi[s].z, 1.0f };
        glLightfv(GL_LIGHT1 + s, GL_POSITION, pos);
    }

    drawSkybox();
    drawTeren();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glDepthMask(GL_FALSE);
    drawMunte();
    glDepthMask(GL_TRUE);

    glPolygonOffset(-2.0f, -2.0f);
    drawDrum();
    glDisable(GL_POLYGON_OFFSET_FILL);

    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-3.0f, -3.0f);

    // umbra de la soare
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    for (int i = 0; i < nrCopaci; i++) {
        float n[3]; getNormalaTeren(copaci[i].x, copaci[i].z, n);
        float pB[3] = { copaci[i].x, calculInaltime(copaci[i].x, copaci[i].z), copaci[i].z };
        glPushMatrix();
        aplicaMatriceUmbra(pB, n, globalLightPos);
        drawCopac(copaci[i].x, copaci[i].z, copaci[i].scara, true, 0.35f);
        glPopMatrix();
    }

    for (int s = 0; s < nrStalpi; s++) {
        float n[3]; getNormalaTeren(stalpi[s].x, stalpi[s].z, n);
        float pB[3] = { stalpi[s].x, calculInaltime(stalpi[s].x, stalpi[s].z), stalpi[s].z };
        glPushMatrix();
        aplicaMatriceUmbra(pB, n, globalLightPos);
        drawStalp(stalpi[s].x, stalpi[s].z, true);
        glPopMatrix();
    }

    // umbrele de la stalpi
    for (int s = 0; s < nrStalpi; s++) {
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_EQUAL, 0, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

        float hStalpFictiv = calculInaltime(stalpi[s].x, stalpi[s].z) + 12.0f;
        float luminaStalpShadow[4] = { stalpi[s].x, hStalpFictiv, stalpi[s].z, 1.0f };
        float distantaMaxima = 25.0f;

        for (int i = 0; i < nrCopaci; i++) {
            float dX = copaci[i].x - stalpi[s].x;
            float dZ = copaci[i].z - stalpi[s].z;
            float distanta = sqrt(dX * dX + dZ * dZ);

            if (distanta < distantaMaxima) {
                float shadowAlpha = 0.6f * (1.0f - (distanta / distantaMaxima));
                if (shadowAlpha < 0.0f) shadowAlpha = 0.0f;

                float n[3]; getNormalaTeren(copaci[i].x, copaci[i].z, n);
                float pB[3] = { copaci[i].x, calculInaltime(copaci[i].x, copaci[i].z), copaci[i].z };

                glPushMatrix();
                aplicaMatriceUmbra(pB, n, luminaStalpShadow);
                drawCopac(copaci[i].x, copaci[i].z, copaci[i].scara, true, shadowAlpha);
                glPopMatrix();
            }
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_STENCIL_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);

    for (int i = 0; i < nrCopaci; i++) {
        drawCopac(copaci[i].x, copaci[i].z, copaci[i].scara, false);
    }
    for (int s = 0; s < nrStalpi; s++) {
        drawStalp(stalpi[s].x, stalpi[s].z, false);
    }

    drawMasina();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    width = w; height = h;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)width / (double)height, 0.1, 300.0);
    glMatrixMode(GL_MODELVIEW);
}

void idle() {
    float yawRad = camYaw * M_PI / 180.0f;
    float pitchRad = camPitch * M_PI / 180.0f;
    float frontX = cos(pitchRad) * cos(yawRad); float frontY = sin(pitchRad); float frontZ = cos(pitchRad) * sin(yawRad);
    float rightX = cos(yawRad + M_PI / 2.0f); float rightZ = sin(yawRad + M_PI / 2.0f);

    if (keys['w'] || keys['W']) { camX += frontX * camSpeed; camY += frontY * camSpeed; camZ += frontZ * camSpeed; }
    if (keys['s'] || keys['S']) { camX -= frontX * camSpeed; camY -= frontY * camSpeed; camZ -= frontZ * camSpeed; }
    if (keys['a'] || keys['A']) { camX -= rightX * camSpeed; camZ -= rightZ * camSpeed; }
    if (keys['d'] || keys['D']) { camX += rightX * camSpeed; camZ += rightZ * camSpeed; }
    if (keys[' '])  camY += camSpeed;
    if (keys['q'] || keys['Q']) camY -= camSpeed;

    // accelerare / franare
    if (keySpecial[GLUT_KEY_UP]) {
        masinaSpeed += 0.02f;
        if (masinaSpeed > masinaMaxSpeed) masinaSpeed = masinaMaxSpeed;
    } else if (keySpecial[GLUT_KEY_DOWN]) {
        masinaSpeed -= 0.02f;
        if (masinaSpeed < -masinaMaxSpeed / 2.0f) masinaSpeed = -masinaMaxSpeed / 2.0f;
    } else {
        if (masinaSpeed > 0.0f) masinaSpeed -= 0.01f;
        if (masinaSpeed < 0.0f) masinaSpeed += 0.01f;
        if (fabs(masinaSpeed) < 0.01f) masinaSpeed = 0.0f;
    }

    // viraj
    if (fabs(masinaSpeed) > 0.0f) {
        float directie = (masinaSpeed > 0.0f) ? 1.0f : -1.0f;
        if (keySpecial[GLUT_KEY_LEFT])  masinaYaw -= 2.5f * directie;
        if (keySpecial[GLUT_KEY_RIGHT]) masinaYaw += 2.5f * directie;
    }

    float radMasina = masinaYaw * M_PI / 180.0f;
    float viitorX = masinaX + cos(radMasina) * masinaSpeed;
    float viitorZ = masinaZ + sin(radMasina) * masinaSpeed;

    if (!verificaColiziune(viitorX, viitorZ)) {
        masinaX = viitorX;
        masinaZ = viitorZ;
    } else {
        masinaSpeed = 0.0f;
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);
    keys[key] = true;
}

void keyboardUp(unsigned char key, int x, int y) { keys[key] = false; }

void specialKeys(int key, int x, int y) {
    if (key >= 0 && key < 256) keySpecial[key] = true;
}

void specialKeysUp(int key, int x, int y) {
    if (key >= 0 && key < 256) keySpecial[key] = false;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Proiect Luciano: Circuitul de munte");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);

    glEnable(GL_NORMALIZE);

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    float ambientModel[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientModel);

    glEnable(GL_LIGHT0);
    float diffuseSoare[] = { 0.7f, 0.7f, 0.7f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseSoare);

    float stalpDiffuse[] = { 1.5f, 1.2f, 0.6f, 1.0f };
    for (int i = 0; i < 3; i++) {
        glEnable(GL_LIGHT1 + i);
        glLightfv(GL_LIGHT1 + i, GL_DIFFUSE, stalpDiffuse);
        glLightf(GL_LIGHT1 + i, GL_CONSTANT_ATTENUATION, 0.5f);
        glLightf(GL_LIGHT1 + i, GL_LINEAR_ATTENUATION, 0.02f);
    }

    glEnable(GL_TEXTURE_2D);

    texIarba = LoadTexture("iarba.jpg");
    texOrizont = LoadTexture("horizont.jpg");
    texPiatra = LoadTexture("piatra.jpg");
    texAsfalt = LoadTexture("drum.jpg");
    texFrunze = LoadTexture("frunze.jpg");
    texTrunk = LoadTexture("trunchi.jpg");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouseClick);
    glutMotionFunc(mouseMotionDrag);

    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);

    genereazaCopaci();

    // inregistrarea stalpilor ca obstacole
    for (int s = 0; s < nrStalpi; s++) {
        adaugaObstacol(stalpi[s].x, stalpi[s].z, 0.3f, 0.3f);
    }

    // inregistrarea copacilor ca obstacole
    for (int i = 0; i < nrCopaci; i++) {
        float diametruTrunchi = 0.3f * copaci[i].scara;
        adaugaObstacol(copaci[i].x, copaci[i].z, diametruTrunchi, diametruTrunchi);
    }

    glutMainLoop();
    return 0;
}