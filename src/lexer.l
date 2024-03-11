INDEX   ([1-9]+[0-9]*)
INTEGER (0|([-+]?[1-9]+[0-9]*))
EXP	    ([Ee][-+]?[0-9]+)
FLOAT   (([-+]?[0-9]+"."[0-9]*{EXP}?)|({INTEGER}))

%x VERTEX
%x FACE

%{
#include <stdbool.h>
#include <glib.h>
#include <cglm/cglm.h>
#include "vk.h"

typedef struct {
  float x;
  float y;
  float z;
} vertex;

typedef struct {
  // list of indices
  GList *loi;
} face;

GArray *objVertices;
GArray *faces;
int numIndices;

// GArray *vertices;
// GArray *indices;

Vertex *vertices;
uint32_t *indices;

// const Vertex vertices[] = {{{-1.25f, -1.25f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  {{+1.25f, -1.25f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
//                            {{+1.25f, +1.25f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  {{-1.25f, +1.25f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
//                            {{-1.25f, -1.25f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, {{+1.25f, -1.25f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f,
//                            0.0f}},
//                            {{+1.25f, +1.25f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, {{-1.25f, +1.25f, -0.5f}, {1.0f, 1.0f, 1.0f},
//                            {1.0f, 1.0f}}};

// const uint16_t indices[] = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

void addVertex(char* f);
void addFace(char* i);

%}

%%

^v" "+                     { BEGIN(VERTEX); }
^f" "+                     { BEGIN(FACE); }
\n                         { yylineno++; }
.

<VERTEX>{FLOAT}(" "+{FLOAT}" "*){2} { addVertex(g_strstrip(yytext)); BEGIN(INITIAL); }
<VERTEX>.                           { printf("<VERTEX>Error in line %d: %s\n", yylineno, yytext); exit(EXIT_FAILURE); }
<FACE>(({INDEX}"/"*){1,3}" "*)+     { addFace(g_strstrip(yytext));   BEGIN(INITIAL); }
<FACE>.                             { printf("<FACE>Error in line %d: %s\n", yylineno, yytext);   exit(EXIT_FAILURE); }

%%

// input: 2.234 1.134 4.234
void addVertex(char* f) {
  gchar **parts = g_regex_split_simple(" +", f, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  float *v = malloc(sizeof(vertex));
  vertex *vCpy = (vertex*)v;
  while (*parts) {
    float val = atof(*parts++);
    *v++ = val;
  }
  g_array_append_val(objVertices, *vCpy);
}

// input: 5/1/2 4/3/2 3/2/1
void addFace(char* i) {
  gchar **parts1 = g_regex_split_simple(" +", i, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  GList *indices = NULL;
  while (*parts1) {
    gchar **parts2 = g_regex_split_simple("/", *parts1, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
    int index = atoi(*parts2);
    indices = g_list_append(indices, GINT_TO_POINTER(index));
    parts1++;
  }
  face *f = malloc(sizeof(face));
  f->loi = indices;
  g_array_append_val(faces, *f);
}

void printVertices() {
  for(int i = 0; i < objVertices->len; i++) {
   vertex *v = &g_array_index(objVertices, vertex, i);
   printf("Vertex #%d: %f, %f, %f\n", i + 1, v->x, v->y, v->z);
  }
}

void printFaces() {
  printf("\n");
  for(int i = 0; i < faces->len; i++) {
    face f = g_array_index(faces, face, i);
    printf("Face #%d: ", i);
    GList* l = f.loi;
    while(l) {
      printf("%d ", GPOINTER_TO_INT(l->data));
      l = l->next;
    }
    printf("\n");
  }
}

void createVertices() {
  vertices = malloc(objVertices->len * sizeof(Vertex));
  Vertex *verticesCpy = vertices;
  for(int i = 0; i < objVertices->len; i++) {
   vertex *v = &g_array_index(objVertices, vertex, i);
   verticesCpy[i].pos[0]   = v->x;
   verticesCpy[i].pos[1]   = v->y;
   verticesCpy[i].pos[2]   = v->z;
   verticesCpy[i].color[0] = 1.0f;
   verticesCpy[i].color[1] = 0.0f;
   verticesCpy[i].color[2] = 0.0f;
   // texture coordinates are not used
   // …
  }
}

int countIndices() {
  unsigned int sum = 0;
  for(int i = 0; i < faces->len; i++) {
    face f = g_array_index(faces, face, i);
    GList* l = f.loi;
    sum += g_list_length (l);
  }
  printf("\nNumber of indices: %d\n", sum);
  return sum;
}

void createIndices() {
  numIndices = countIndices();
  indices = malloc(numIndices * sizeof(uint32_t));
  for(int i = 0, j = 0; i < faces->len; i++) {
    face f = g_array_index(faces, face, i);
    GList* l = f.loi;
    while(l) {
      indices[j++] = (uint32_t) GPOINTER_TO_INT(l->data) - 1;
      l = l->next;
    }
  }
}

void LoadModel(void) {
  yyin = fopen("models/dodecahedron.obj", "r");
  if(!yyin) {
    perror("couldn't open file");
  }
  objVertices = g_array_new(FALSE, FALSE, sizeof(vertex));
  faces       = g_array_new(FALSE, FALSE, sizeof(face));;
  yylex();
  printVertices();
  printFaces();
  createVertices();
  createIndices();
}
