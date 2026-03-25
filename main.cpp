#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GLUT/glut.h>
#include <cmath>
#include <cstdlib>

static int width = 800;
static int height = 600;

static float angle = 0.0f;

GLuint texIarba;
GLuint texOrizont;
GLuint texPiatra;
GLuint texAsfalt;

GLuint texFrunze;
GLuint texTrunk;

struct Copac {
    float x, z, scara;
};

#define MAX_COPACI 200
Copac copaci[MAX_COPACI];
int nrCopaci = 0;

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
        printf("Texture %s loaded: %dx%d, %d channels\n", filename, w, h, nrChannels);
    } else {
        printf("ERROR: Couldn't load %s\n", filename);
    }

    stbi_image_free(data);
    return texture;
}

float calcAlphaMunte(float x, float z) {
    float mx = x + 40.0f;
    float mz = z + 40.0f;
    float dm = sqrt(mx * mx + mz * mz);
    float unghi = atan2(mz, mx);
    float raza = 14.0f + 3.0f * sin(unghi * 3.0f) + 2.0f * cos(unghi * 5.0f + 1.0f);

    if (dm >= raza) return 0.0f;

    float t = dm / raza;       // 0 = centru, 1 = margine
    if (t < 0.5f) return 1.0f;
    return 1.0f - (t - 0.5f) / 0.5f;  // fade spre iarba
}

float calculInaltime(float x, float z) {
    float y = 0.0f;

    // teren ondulat de baza
    y += 1.5f * sin(x * 0.07f + 1.3f) * cos(z * 0.09f + 0.7f);
    y += 0.8f * cos(x * 0.13f - z * 0.11f + 2.1f);
    y += 0.4f * sin(x * 0.21f + z * 0.17f - 0.5f);

    // muntele
    float mx = x + 40.0f;
    float mz = z + 40.0f;
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

void drawTeren() {
    glBindTexture(GL_TEXTURE_2D, texIarba);
    glColor3f(0.15f, 0.3f, 0.15f);

    float dim = 50.0f, pas = 1.0f;

    glBegin(GL_QUADS);
    for (float z = -dim; z < dim; z += pas) {
        for (float x = -dim; x < dim; x += pas) {
            float y1 = calculInaltime(x, z);
            float y2 = calculInaltime(x + pas, z);
            float y3 = calculInaltime(x + pas, z + pas);
            float y4 = calculInaltime(x, z + pas);

            float u1 = (x + dim) / 5.0f, v1 = (z + dim) / 5.0f;
            float u2 = (x + pas + dim) / 5.0f, v2 = (z + pas + dim) / 5.0f;

            glTexCoord2f(u1, v1); glVertex3f(x,       y1, z);
            glTexCoord2f(u2, v1); glVertex3f(x + pas, y2, z);
            glTexCoord2f(u2, v2); glVertex3f(x + pas, y3, z + pas);
            glTexCoord2f(u1, v2); glVertex3f(x,       y4, z + pas);
        }
    }
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
}

void drawMunte() {
    glBindTexture(GL_TEXTURE_2D, texPiatra);

    float centruX = -40.0f, centruZ = -40.0f;
    float razaMax = 19.0f;
    float pas = 1.0f;
    float dim = 50.0f;

    glBegin(GL_QUADS);
    for (float z = centruZ - razaMax; z < centruZ + razaMax; z += pas) {
        for (float x = centruX - razaMax; x < centruX + razaMax; x += pas) {
            float a1 = calcAlphaMunte(x, z);
            float a2 = calcAlphaMunte(x + pas, z);
            float a3 = calcAlphaMunte(x + pas, z + pas);
            float a4 = calcAlphaMunte(x, z + pas);

            if (a1 == 0.0f && a2 == 0.0f && a3 == 0.0f && a4 == 0.0f)
                continue;

            float y1 = calculInaltime(x, z);
            float y2 = calculInaltime(x + pas, z);
            float y3 = calculInaltime(x + pas, z + pas);
            float y4 = calculInaltime(x, z + pas);

            float u1 = (x + dim) / 3.0f, v1 = (z + dim) / 3.0f;
            float u2 = (x + pas + dim) / 3.0f, v2 = (z + pas + dim) / 3.0f;

            glColor4f(0.7f, 0.7f, 0.7f, a1);
            glTexCoord2f(u1, v1); glVertex3f(x,       y1, z);
            glColor4f(0.7f, 0.7f, 0.7f, a2);
            glTexCoord2f(u2, v1); glVertex3f(x + pas, y2, z);
            glColor4f(0.7f, 0.7f, 0.7f, a3);
            glTexCoord2f(u2, v2); glVertex3f(x + pas, y3, z + pas);
            glColor4f(0.7f, 0.7f, 0.7f, a4);
            glTexCoord2f(u1, v2); glVertex3f(x,       y4, z + pas);
        }
    }
    glEnd();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void drawDrum() {
    glBindTexture(GL_TEXTURE_2D, texAsfalt);
    glColor3f(0.9f, 0.9f, 0.9f);

    float a = 25.0f;   // pe X
    float b = 18.0f;  // pe Z
    float latimeDrum = 4.0f;

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

        float len1 = sqrt(tx1 * tx1 + tz1 * tz1);
        float nx1 = tz1 / len1, nz1 = -tx1 / len1;

        float len2 = sqrt(tx2 * tx2 + tz2 * tz2);
        float nx2 = tz2 / len2, nz2 = -tx2 / len2;

        float half = latimeDrum / 2.0f;

        float x1i = cx1 - nx1 * half, z1i = cz1 - nz1 * half;
        float x1e = cx1 + nx1 * half, z1e = cz1 + nz1 * half;
        float x2i = cx2 - nx2 * half, z2i = cz2 - nz2 * half;
        float x2e = cx2 + nx2 * half, z2e = cz2 + nz2 * half;

        float y1i = calculInaltime(x1i, z1i) + 0.05f;
        float y1e = calculInaltime(x1e, z1e) + 0.05f;
        float y2i = calculInaltime(x2i, z2i) + 0.05f;
        float y2e = calculInaltime(x2e, z2e) + 0.05f;

        float dx = cx2 - cx1, dz = cz2 - cz1;
        float lungSegment = sqrt(dx * dx + dz * dz);
        float vUrm = vAcum + lungSegment / latimeDrum;

        glTexCoord2f(0.0f, vAcum); glVertex3f(x1i, y1i, z1i);
        glTexCoord2f(1.0f, vAcum); glVertex3f(x1e, y1e, z1e);
        glTexCoord2f(1.0f, vUrm);  glVertex3f(x2e, y2e, z2e);
        glTexCoord2f(0.0f, vUrm);  glVertex3f(x2i, y2i, z2i);

        vAcum = vUrm;
    }
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
}

void drawCopac(float x, float z, float scara) {
    float y = calculInaltime(x, z);

    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scara, scara, scara);

    glBindTexture(GL_TEXTURE_2D, texTrunk);
    glColor3f(0.6f, 0.5f, 0.4f);

    float rT = 0.2f;
    float hT = 1.8f;
    int seg = 10;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= seg; i++) {
        float t = (float)i / seg * 2.0f * M_PI;
        float cx = rT * cos(t);
        float cz = rT * sin(t);
        float u = (float)i / seg * 2.0f;
        glTexCoord2f(u, 0.0f); glVertex3f(cx, 0.0f, cz);
        glTexCoord2f(u, 1.0f); glVertex3f(cx, hT,   cz);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, texFrunze);

    int etaje = 5;
    int segCon = 16;
    float startY = hT - 0.3f;

    for (int e = 0; e < etaje; e++) {
        float et = (float)e / etaje;

        float razaBaza = (e == etaje - 1)
            ? 0.5f
            : 2.2f * (1.0f - et * 0.55f);
        float hCon = 2.0f * (1.0f - et * 0.15f);
        float bazaY = startY + e * 1.1f;
        float varfY = bazaY + hCon;

        float varfOfsX = 0.1f * sin(e * 2.3f + scara * 5.0f);
        float varfOfsZ = 0.1f * cos(e * 3.1f + scara * 7.0f);

        float verde = 0.28f + 0.04f * e;
        glColor3f(0.08f + 0.02f * e, verde, 0.06f + 0.015f * e);

        glBegin(GL_TRIANGLE_FAN);
        glTexCoord2f(0.5f, 0.0f);
        glVertex3f(varfOfsX, varfY, varfOfsZ);
        for (int i = 0; i <= segCon; i++) {
            float a = (float)i / segCon * 2.0f * M_PI;
            float rVar = razaBaza * (1.0f + 0.15f * sin(a * 3.0f + e * 1.7f)
                                          + 0.1f * cos(a * 5.0f + e * 2.3f));
            float yVar = bazaY + 0.15f * sin(a * 4.0f + e * 1.1f);

            float u = (float)i / segCon * 5.0f;
            glTexCoord2f(u, 1.0f);
            glVertex3f(rVar * cos(a), yVar, rVar * sin(a));
        }
        glEnd();

        glBegin(GL_TRIANGLE_FAN);
        glTexCoord2f(0.5f, 0.5f);
        glVertex3f(0.0f, bazaY, 0.0f);
        for (int i = segCon; i >= 0; i--) {
            float a = (float)i / segCon * 2.0f * M_PI;
            float rVar = razaBaza * (1.0f + 0.15f * sin(a * 3.0f + e * 1.7f)
                                          + 0.1f * cos(a * 5.0f + e * 2.3f));
            float yVar = bazaY + 0.15f * sin(a * 4.0f + e * 1.1f);
            float u = 0.5f + 0.5f * cos(a);
            float v = 0.5f + 0.5f * sin(a);
            glTexCoord2f(u, v);
            glVertex3f(rVar * cos(a), yVar, rVar * sin(a));
        }
        glEnd();
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void drawSkybox() {
    glBindTexture(GL_TEXTURE_2D, texOrizont);

    float d = 50.0f;
    float yJos = -5.0f;
    float ySus = 50.0f;

    glBegin(GL_QUADS);

    glTexCoord2f(0.0f, 0.0f); glVertex3f(-d, yJos, -d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( d, yJos, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( d, ySus, -d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-d, ySus, -d);

    glTexCoord2f(1.0f, 0.0f); glVertex3f(d, yJos, -d);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(d, yJos,  d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(d, ySus,  d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(d, ySus, -d);

    glTexCoord2f(0.0f, 0.0f); glVertex3f( d, yJos, d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-d, yJos, d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-d, ySus, d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( d, ySus, d);

    glTexCoord2f(1.0f, 0.0f); glVertex3f(-d, yJos,  d);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-d, yJos, -d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-d, ySus, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-d, ySus,  d);

    glEnd();
}

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
        float distElipsa = sqrt(ex * ex + ez * ez);
        if (fabs(distElipsa - 1.0f) < 0.25f)
            continue;

        float dmx = x - centruMX, dmz = z - centruMZ;
        float distMunte = sqrt(dmx * dmx + dmz * dmz);
        if (distMunte < 22.0f)
            continue;

        if (distMunte < 35.0f) {
            if ((float)rand() / RAND_MAX < 0.7f)
                continue;
        }

            bool preAproape = false;
            for (int k = 0; k < nrCopaci; k++) {
                float ddx = copaci[k].x - x;
                float ddz = copaci[k].z - z;
                if (ddx * ddx + ddz * ddz < 9.0f) {
                    preAproape = true;
                    break;
                }
            }
            if (preAproape) continue;

        copaci[nrCopaci].x = x;
        copaci[nrCopaci].z = z;
        copaci[nrCopaci].scara = 0.7f + ((float)rand() / RAND_MAX) * 0.6f;
        nrCopaci++;
    }
    printf("Generati %d copaci\n", nrCopaci);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float cameraX = 40.0f * sin(angle);
    float cameraZ = 40.0f * cos(angle);
    gluLookAt(cameraX, 15.0, cameraZ,
              0.0, 2.0, 0.0,
              0.0, 1.0, 0.0);

    drawSkybox();

    drawTeren();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glDepthMask(GL_FALSE);

    drawMunte();

    glDepthMask(GL_TRUE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_BLEND);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -2.0f);
    drawDrum();
    glDisable(GL_POLYGON_OFFSET_FILL);

    for (int i = 0; i < nrCopaci; i++) {
        drawCopac(copaci[i].x, copaci[i].z, copaci[i].scara);
    }

    glutSwapBuffers();
}

void reshape(int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)width / (double)height, 0.1, 300.0);

    glMatrixMode(GL_MODELVIEW);
}

void idle() {
    angle += 0.001f;
    if (angle >= 360.0f) angle -= 360.0f;
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) {
        exit(0);
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Proiect Luciano: Circuitul de munte");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);

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

    genereazaCopaci();

    glutMainLoop();
    return 0;
}