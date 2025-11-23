#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/freeglut.h>

/* Implementação do carregamento de imagens via stb_image */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* Implementação da lib fast_obj, responsável por carregar OBJ/MTL */
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"

/* Estrutura que representa cada modelo 3D carregado */
typedef struct {
    fastObjMesh* mesh;               // Dados do modelo carregado
    GLuint* materialTextures;        // Array com as texturas carregadas
    int materialCount;               // Quantidade de materiais
    float escala;                    // Escala global do modelo
    float centro[3];                 // Centro geométrico (para centralizar na tela)
    int carregado;                   // Flag indicando se está carregado
    char nome[128];                  // Nome do arquivo
} Objeto3D;

Objeto3D objetos[3];        // Lista com 3 modelos
int modeloAtual = 0;        // Índice do modelo sendo exibido

// Controle de câmera/rotação
float anguloX = 0, anguloY = 0, distCamera = 5;
int ultimoX = 0, ultimoY = 0, botaoPressionado = 0;

/* Verifica se arquivo existe */
static int file_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

/* Carrega textura a partir de arquivo usando stb_image */
GLuint loadTexture(const char *filename) {
    if (!filename) return 0;
    if (!file_exists(filename)) {
        printf("[TEX] nao encontrado: %s\n", filename);
        return 0;
    }

    int w,h,c;
    unsigned char *data = stbi_load(filename, &w, &h, &c, 0);
    if (!data) return 0;

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int fmt = (c == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return id;
}

/* Libera memória associada a um objeto (mesh + texturas) */
void liberarObjeto(Objeto3D* o) {
    if (!o) return;

    if (o->mesh)
        fast_obj_destroy(o->mesh);

    if (o->materialTextures) {
        for (int i=0; i<o->materialCount; i++)
            if (o->materialTextures[i])
                glDeleteTextures(1, &o->materialTextures[i]);
        free(o->materialTextures);
    }

    o->mesh = NULL;
    o->materialTextures = NULL;
    o->materialCount = 0;
    o->carregado = 0;
}

/* Carrega um OBJ usando fast_obj e processa materiais, texturas e bounding box */
void carregarObjeto(int indice, const char* filename) {
    printf("\n[LOAD] %s\n", filename);
    Objeto3D* obj = &objetos[indice];
    liberarObjeto(obj); // limpa dados antigos

    obj->mesh = fast_obj_read(filename);
    if (!obj->mesh) return;

    strncpy(obj->nome, filename, 127);
    obj->carregado = 1;

    /* Prepara texturas baseadas no MTL */
    obj->materialCount = obj->mesh->material_count;
    obj->materialTextures = calloc(obj->materialCount, sizeof(GLuint));

    for (int m = 0; m < obj->materialCount; m++) {
        unsigned int texIndex = obj->mesh->materials[m].map_Kd;
        const char* texName = NULL;

        if (texIndex > 0 && texIndex < obj->mesh->texture_count)
            texName = obj->mesh->textures[texIndex].name;

        obj->materialTextures[m] = texName ? loadTexture(texName) : 0;
    }

    /* Calcula bounding box para centralizar o modelo e ajustar escala */
    float minv[3] = {1e9,1e9,1e9}, maxv[3]={-1e9,-1e9,-1e9};
    for (unsigned i = 0; i < obj->mesh->position_count; i++) {
        float x = obj->mesh->positions[3*i+0];
        float y = obj->mesh->positions[3*i+1];
        float z = obj->mesh->positions[3*i+2];

        if (x<minv[0]) minv[0]=x; if (x>maxv[0]) maxv[0]=x;
        if (y<minv[1]) minv[1]=y; if (y>maxv[1]) maxv[1]=y;
        if (z<minv[2]) minv[2]=z; if (z>maxv[2]) maxv[2]=z;
    }

    obj->centro[0] = (minv[0] + maxv[0]) / 2;
    obj->centro[1] = (minv[1] + maxv[1]) / 2;
    obj->centro[2] = (minv[2] + maxv[2]) / 2;

    float dx = maxv[0]-minv[0], dy = maxv[1]-minv[1], dz = maxv[2]-minv[2];
    float md = dx;

    if (dy > md) md = dy;
    if (dz > md) md = dz;

    obj->escala = 4.0f / md; 
}

/* Define aparência do objeto, incluindo textura e materiais especiais */
void aplicarEstiloVisual(Objeto3D* obj, int matIndex) {
    GLuint tex = 0;

    if (matIndex >= 0 && matIndex < obj->materialCount)
        tex = obj->materialTextures[matIndex];

    /* Se houver textura, aplica como DECAL (mantém luz sem escurecer) */
    if (tex) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glColor3f(1,1,1);
        return;
    }

    glDisable(GL_TEXTURE_2D);

    /* Material especial do dragão (efeito pedra) */
    if (strstr(obj->nome, "dragon")) {
        GLfloat stoneAmb[] = {0.30f, 0.30f, 0.30f, 1.0f};
        GLfloat stoneDiff[] = {0.55f, 0.55f, 0.55f, 1.0f};
        GLfloat stoneSpec[] = {0.10f, 0.10f, 0.10f, 1.0f};

        glMaterialfv(GL_FRONT, GL_AMBIENT, stoneAmb);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, stoneDiff);
        glMaterialfv(GL_FRONT, GL_SPECULAR, stoneSpec);
        glMaterialf(GL_FRONT, GL_SHININESS, 6.0f);

        glColor3f(0.55f, 0.55f, 0.55f);
        return;
    }

    /* Material padrão */
    GLfloat def[] = {0.8,0.8,0.8,1};
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, def);
    glMaterialf(GL_FRONT, GL_SHININESS, 40);
}

/* Renderiza a cena */
void display() {
    Objeto3D* obj = &objetos[modeloAtual];
    if (!obj->carregado) return;

    /* Fundo diferenciado para o dragão */
    if (strstr(obj->nome, "dragon"))
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    else
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    GLfloat luzPos[] = {5,10,5,1};
    glLightfv(GL_LIGHT0, GL_POSITION, luzPos);

    /* Configura câmera */
    gluLookAt(0,0,distCamera, 0,0,0, 0,1,0);

    glPushMatrix();
    glRotatef(anguloX,1,0,0);
    glRotatef(anguloY,0,1,0);
    glScalef(obj->escala, obj->escala, obj->escala);
    glTranslatef(-obj->centro[0], -obj->centro[1], -obj->centro[2]);

    /* Renderização baseada nos índices do fast_obj */
    unsigned idxOffset = 0;
    for (unsigned f = 0; f < obj->mesh->face_count; f++) {
        int fv = obj->mesh->face_vertices[f];
        int mat = obj->mesh->face_materials ? obj->mesh->face_materials[f] : -1;

        aplicarEstiloVisual(obj, mat);

        glBegin(fv == 3 ? GL_TRIANGLES : GL_POLYGON);
        for (int v = 0; v < fv; v++) {
            fastObjIndex idx = obj->mesh->indices[idxOffset + v];

            if (idx.n >= 0)
                glNormal3fv(&obj->mesh->normals[3 * idx.n]);

            if (idx.t >= 0)
                glTexCoord2f(obj->mesh->texcoords[2 * idx.t],
                             1 - obj->mesh->texcoords[2 * idx.t + 1]);

            glVertex3fv(&obj->mesh->positions[3 * idx.p]);
        }
        glEnd();

        idxOffset += fv;
    }

    glPopMatrix();
    glutSwapBuffers();
}

/* Troca de modelos usando teclado */
void keyboardFunc(unsigned char key,int x,int y){
    if (key == '1') modeloAtual = 0;
    if (key == '2') modeloAtual = 1;
    if (key == '3') modeloAtual = 2;
    if (key == 27) exit(0); // ESC
    glutPostRedisplay();
}

/* Zoom através da roda do mouse */
void mouseWheel(int w,int d,int x,int y){
    distCamera += (d>0)?-0.3:0.3;
    if (distCamera < 1) distCamera = 1;
    glutPostRedisplay();
}

/* Controle de rotação via mouse */
void mouseFunc(int b,int s,int x,int y){
    if (b == 3 || b == 4)
        mouseWheel(0,(b==3)?1:-1,x,y);
    else if (b == GLUT_LEFT_BUTTON){
        botaoPressionado = (s == GLUT_DOWN);
        ultimoX = x;
        ultimoY = y;
    }
}

void motionFunc(int x, int y) {
    if (botaoPressionado) {
        anguloY += (x - ultimoX) * 0.5f;
        anguloX += (y - ultimoY) * 0.5f;
        ultimoX = x;
        ultimoY = y;
        glutPostRedisplay();
    }
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w/h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void initGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    GLfloat luzAmb[] = {0.4f, 0.4f, 0.4f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luzAmb);
}

int main(int argc, char** argv) {
    for (int i = 0; i < 3; i++) {
        objetos[i].mesh = NULL;
        objetos[i].materialTextures = NULL;
        objetos[i].materialCount = 0;
        objetos[i].carregado = 0;
        objetos[i].nome[0] = '\0';
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GL_DEPTH);
    glutInitWindowSize(900,600);
    glutCreateWindow("Visualizador OBJ+MTL (robusto)");

    initGL();

    if (argc > 1)
        carregarObjeto(0, argv[1]);
    else
        carregarObjeto(0, "teapot.obj");

    carregarObjeto(1, "bunny.obj");
    carregarObjeto(2, "dragon.obj");

    printf("\n=== CONTROLES ===\n");
    printf("Tecle [1]: Visualizar Modelo 1 (Padrao: Bule)\n");
    printf("Tecle [2]: Visualizar Modelo 2 (Padrao: Coelho)\n");
    printf("Tecle [3]: Visualizar Modelo 3 (Padrao: Dragao)\n");
    printf("Mouse Esq: Girar | Scroll: Zoom\n");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardFunc);
    glutMouseFunc(mouseFunc);
    glutMotionFunc(motionFunc);
    glutMouseWheelFunc(mouseWheel);

    glutMainLoop();

    
    for (int i = 0; i < 3; i++) liberarObjeto(&objetos[i]);
    return 0;
}
