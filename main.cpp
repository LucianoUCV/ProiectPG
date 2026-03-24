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

GLuint LoadTexture(const char* filename) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int w, h, nrChannels;
    unsigned char* data = stbi_load(filename, &w, &h, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        printf("Texture %s loaded: %dx%d, %d channels\n", filename, w, h, nrChannels);
    } else {
        printf("ERROR: Could'nt load %s\n", filename);
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

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}