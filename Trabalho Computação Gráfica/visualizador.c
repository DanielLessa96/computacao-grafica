#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* * Define as implementacoes das bibliotecas de arquivo unico.
 * stb_image: para carregar texturas (PNG/JPG).
 * fast_obj: para carregar a geometria dos arquivos .obj.
 */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h" 

/* * Estrutura principal para armazenar os dados de cada objeto 3D.
 * Guarda a malha geometrica, o ID da textura OpenGL, informacoes de escala/centro
 * e o nome do arquivo para identificacao.
 */
typedef struct {
    fastObjMesh* mesh;
    GLuint idTextura;
    float escala;
    float centro[3];
    int carregado; 
    char nome[64]; 
} Objeto3D;

/* Variaveis globais para gerenciar os 3 slots de modelos e o estado da camera */
Objeto3D objetos[3]; 
int modeloAtual = 0; 

float anguloX = 0.0f, anguloY = 0.0f, distCamera = 5.0f;
int ultimoX = 0, ultimoY = 0, botaoPressionado = 0;

/*
 * Funcao: loadTexture
 * Objetivo: Carrega uma imagem do disco e a converte em uma textura utilizavel pelo OpenGL.
 * Configura os parametros de repeticao e filtro linear para suavizacao.
 */
GLuint loadTexture(const char *filename) {
    int w, h, c;
    unsigned char *data = stbi_load(filename, &w, &h, &c, 0);
    GLuint id = 0;
    if (data) {
        glGenTextures(1, &id); glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int fmt = (c==4)?GL_RGBA:GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        printf("[OK] Textura '%s' carregada.\n", filename);
    } else {
        printf("[AVISO] Textura '%s' nao encontrada.\n", filename);
    }
    return id;
}

/*
 * Funcao: carregarObjeto
 * Objetivo: Le o arquivo .obj usando a biblioteca fast_obj.
 * Alem de ler a geometria, ela percorre todos os vertices para calcular o "Bounding Box" (caixa limite).
 * Isso serve para encontrar o centro do objeto e calcular a escala necessaria para que ele caiba na tela.
 */
void carregarObjeto(int indice, const char* filename) {
    printf("Carregando Slot %d: '%s'...\n", indice+1, filename);
    
    Objeto3D* obj = &objetos[indice];
    
    if(obj->mesh) fast_obj_destroy(obj->mesh);
    
    obj->mesh = fast_obj_read(filename);
    
    if (!obj->mesh) {
        printf("   [FALHA] Nao foi possivel abrir '%s'. Slot %d ficara vazio.\n", filename, indice+1);
        obj->carregado = 0;
        return;
    }

    strcpy(obj->nome, filename);
    obj->carregado = 1;
    obj->idTextura = 0; 

    float min[3] = {1e9, 1e9, 1e9};
    float max[3] = {-1e9, -1e9, -1e9};

    for (unsigned int i = 0; i < obj->mesh->position_count; i++) {
        float x = obj->mesh->positions[3 * i + 0];
        float y = obj->mesh->positions[3 * i + 1];
        float z = obj->mesh->positions[3 * i + 2];

        if (x < min[0]) min[0] = x; if (x > max[0]) max[0] = x;
        if (y < min[1]) min[1] = y; if (y > max[1]) max[1] = y;
        if (z < min[2]) min[2] = z; if (z > max[2]) max[2] = z;
    }

    obj->centro[0] = (max[0] + min[0]) / 2.0f;
    obj->centro[1] = (max[1] + min[1]) / 2.0f;
    obj->centro[2] = (max[2] + min[2]) / 2.0f;

    float maxDim = max[0] - min[0];
    if ((max[1] - min[1]) > maxDim) maxDim = max[1] - min[1];
    if ((max[2] - min[2]) > maxDim) maxDim = max[2] - min[2];

    if (maxDim > 0) obj->escala = 4.0f / maxDim;
    else obj->escala = 1.0f;

    printf("   [SUCESSO] %d faces.\n", obj->mesh->face_count);
}

/*
 * Funcao: aplicarEstiloVisual
 * Objetivo: Define as propriedades do material (cor, brilho, textura) e a cor de fundo da janela
 * baseando-se no nome do arquivo carregado.
 */
void aplicarEstiloVisual(Objeto3D* obj) {
    if (strstr(obj->nome, "teapot")) {
        if (obj->idTextura > 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, obj->idTextura);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); 
            glColor3f(1,1,1);
        } else {
            glDisable(GL_TEXTURE_2D);
            GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f};
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);
            glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);
        }
    }
    else if (strstr(obj->nome, "dragon")) {
        glDisable(GL_TEXTURE_2D);
        GLfloat matAmb[] = {0.2f, 0.2f, 0.2f, 1.0f};
        GLfloat matDif[] = {0.5f, 0.5f, 0.5f, 1.0f}; 
        glMaterialfv(GL_FRONT, GL_AMBIENT, matAmb);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, matDif);
        glMaterialf(GL_FRONT, GL_SHININESS, 10.0f); 
    }
    
    else if (strstr(obj->nome, "bunny")) {   
        glDisable(GL_TEXTURE_2D);
        GLfloat clay[] = {0.7f, 0.7f, 0.7f, 1.0f}; 
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, clay);
        glMaterialf(GL_FRONT, GL_SHININESS, 5.0f); 
    }
    else {
        glClearColor(0.7f, 0.8f, 0.9f, 1.0f); 
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.8f, 0.8f, 0.8f); 
        GLfloat def[] = {0.8f, 0.8f, 0.8f, 1.0f};
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, def);
    }
}

/*
 * Funcao: display
 * Objetivo: Loop de renderizacao. Limpa a tela, posiciona a luz e camera, aplica transformacoes
 * e desenha a malha do objeto.
 */
void display() {
    Objeto3D* obj = &objetos[modeloAtual];

    /* Corrige a cor do fundo do dragao se necessario antes de limpar o buffer */
    if (strstr(obj->nome, "dragon")){
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    }
    else {
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    aplicarEstiloVisual(obj); 
    
    if (!obj->carregado) {
        glutSwapBuffers();
        return;
    }

    GLfloat luzPos[] = {0.0f, 10.0f, 10.0f, 0.0f}; 
    glLightfv(GL_LIGHT0, GL_POSITION, luzPos);

    gluLookAt(0.0, 0.0, distCamera, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    glPushMatrix();
    glRotatef(anguloX, 1.0, 0.0, 0.0);
    glRotatef(anguloY, 0.0, 1.0, 0.0);
    glScalef(obj->escala, obj->escala, obj->escala);
    glTranslatef(-obj->centro[0], -obj->centro[1], -obj->centro[2]);

    /* Desenho da geometria:
       Itera sobre todas as faces. Se a face tiver mais de 3 vertices (ex: quadrado),
       usa GL_POLYGON para desenhar corretamente. Se for triangulo padrao, usa GL_TRIANGLES.
    */
    glBegin(GL_TRIANGLES);
    unsigned int index_offset = 0;
    for (unsigned int i = 0; i < obj->mesh->face_count; i++) {
        int faceVertices = obj->mesh->face_vertices[i]; 
        
        if (faceVertices != 3) { glEnd(); glBegin(GL_POLYGON); } 

        for (int j = 0; j < faceVertices; j++) {
            fastObjIndex idx = obj->mesh->indices[index_offset + j];

            if (idx.n) {
                glNormal3f(obj->mesh->normals[3 * idx.n + 0],
                           obj->mesh->normals[3 * idx.n + 1],
                           obj->mesh->normals[3 * idx.n + 2]);
            }
            if (idx.t && obj->idTextura > 0) {
                glTexCoord2f(obj->mesh->texcoords[2 * idx.t + 0],
                             1.0f - obj->mesh->texcoords[2 * idx.t + 1]); 
            }
            glVertex3f(obj->mesh->positions[3 * idx.p + 0],
                       obj->mesh->positions[3 * idx.p + 1],
                       obj->mesh->positions[3 * idx.p + 2]);
        }
        
        if (faceVertices != 3) { glEnd(); glBegin(GL_TRIANGLES); }
        
        index_offset += faceVertices; 
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    glutSwapBuffers();
}

/* Callbacks para interacao com teclado e mouse */
void keyboardFunc(unsigned char key, int x, int y) {
    if(key == 27) exit(0);
    
    /* Alterna entre os slots de modelos carregados */
    if(key == '1') { modeloAtual = 0; printf("Visualizando: Modelo 1 (Bule)\n"); }
    if(key == '2') { modeloAtual = 1; printf("Visualizando: Modelo 2 (Coelho)\n"); }
    if(key == '3') { modeloAtual = 2; printf("Visualizando: Modelo 3 (Dragao)\n"); }
    
    glutPostRedisplay();
}

void mouseWheel(int wheel, int direction, int x, int y) {
    distCamera += (direction > 0) ? -0.5f : 0.5f;
    if(distCamera < 1.0f) distCamera=1.0f; 
    glutPostRedisplay();
}
void mouseFunc(int button, int state, int x, int y) {
    if (button == 3 || button == 4) mouseWheel(0, (button==3)?1:-1, x, y);
    else if (button == GLUT_LEFT_BUTTON) { botaoPressionado = (state == GLUT_DOWN); ultimoX = x; ultimoY = y; }
}
void motionFunc(int x, int y) {
    if (botaoPressionado) { anguloY += (x-ultimoX)*0.5f; anguloX += (y-ultimoY)*0.5f; ultimoX=x; ultimoY=y; glutPostRedisplay(); }
}
void reshape(int w, int h) {
    glViewport(0, 0, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45, (float)w/h, 0.1, 100.0); glMatrixMode(GL_MODELVIEW);
}

void initGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    GLfloat luzAmb[] = {0.4f, 0.4f, 0.4f, 1.0f}; 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luzAmb);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Visualizador 3D");

    initGL();

    /* Carregamento inicial dos 3 modelos padrao */
    
    if (argc > 1) {
        /* Se o usuario passou um arquivo, carrega no slot 1 (substitui bule) */
        carregarObjeto(0, argv[1]);
        carregarObjeto(1, "bunny.obj");
        carregarObjeto(2, "dragon.obj");
    } 
    else {
        /* Carregamento padrao */
        carregarObjeto(0, "teapot.obj");
        carregarObjeto(1, "bunny.obj");
        carregarObjeto(2, "dragon.obj");
    }

    /* Tenta carregar a textura default para o bule, se ele estiver no slot 0 */
    if (strstr(objetos[0].nome, "teapot")) {
        objetos[0].idTextura = loadTexture("default.png");
    }

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
    
    for(int i=0; i<3; i++) if(objetos[i].mesh) fast_obj_destroy(objetos[i].mesh);
    return 0;
}