# Visualizador de modelos 3D

Um visualizador de modelos 3D leve e eficiente desenvolvido em C e OpenGL, capaz de carregar arquivos Wavefront (.obj) complexos e aplicar texturas ou materiais adaptativos automaticamente.

![Demo do Projeto](animacao.gif)

## 📋 Sobre o Projeto

Este projeto foi desenvolvido como trabalho final da disciplina de **Computação Gráfica**. O objetivo é renderizar cenas 3D a partir de arquivos OBJ, garantindo performance e tratamento correto de geometria e iluminação.

### Principais Funcionalidades
* **Carregamento Robusto:** Utiliza a biblioteca `fast_obj` para carregar malhas densas (como o Dragão de Stanford) instantaneamente.
* **Texturização:** Suporte a texturas PNG/JPG via `stb_image`.
* **Renderização Adaptativa:** O software detecta o modelo carregado e ajusta automaticamente:
    * **Iluminação:** (Ex: Correção `GL_DECAL` para texturas brilhantes).
    * **Materiais:** (Ex: Pedra para o dragão, Argila para o coelho).
    * **Ambiente:** Cor de fundo dinâmica para melhor contraste.
* **Câmera Interativa:** Sistema *Arcball* simplificado para rotação e zoom infinito.
* **Auto-ajuste:** Centralização e escala automática do modelo na tela.

## 🚀 Como Executar

### Pré-requisitos
Você precisará das bibliotecas de desenvolvimento do OpenGL e FreeGLUT instaladas.
* **Linux (Debian/Ubuntu):** `sudo apt-get install freeglut3-dev`
* **Windows:** MinGW com FreeGLUT configurado.

### Compilação
No terminal, execute:
`gcc visualizador.c -o visualizador -lGL -lGLU -lglut -lm`

### Uso 
Você pode rodar o programa de duas formas:
* **Modo Padrão** (Carrega Bule, Coelho e Dragão):
`./visualizador`
* **Modo Arquivo Específico** (Carrega seu modelo no Slot 1):
`./visualizador meu_arquivo.obj`

### 🎮 Controles
Tecla / Ação Função
* **Tecla 1:** Visualizar Modelo 1 (Bule Texturizado)
* **Tecla 2:** Visualizar Modelo 2 (Coelho de Argila)
* **Tecla 3:** Visualizar Modelo 3 (Dragão de Pedra)
* **Mouse Esq.:** + Arraste Rotacionar o objeto 
* **Scroll:** do MouseZoom (Aproximar/Afastar)
* **ESC:** Fechar o programa



### 🛠️ Tecnologias Utilizadas
* **Linguagem C:** Lógica principal.
* **OpenGL / GLU:** Renderização gráfica e pipeline fixo.
* **FreeGLUT:** Gerenciamento de janelas e input.
* **fast_obj:** Biblioteca para parsing rápido e robusto de arquivos OBJ.
* **stb_image:** Biblioteca header-only para carregamento de imagens (PNG/JPG).

