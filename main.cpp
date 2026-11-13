#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <set>
#include <map>
#include <queue>
#include <stack>

using namespace std;

//Construccion de arbol de dfs

//hacemos la clase nodo como lo aprendido en clase
class Node {
public:
    string value;            // Lo que viene en la cordenada lo usamos como string: "(fila,columna)"
    pair<int, int> coord;    // Par {fila, columna}
    Node* parent;
    vector<Node*> children;

    Node(int x, int y) {
        coord = {x, y};
        value = "(" + to_string(x) + "," + to_string(y) + ")"; // Tenemos que convertir a string para poder guardarlo en el vector de strings
        parent = nullptr; //al padre lo inicializamos en nullptr, ya que al crear el nodo no tiene padre
    }

    // Se agrega un hijo y a este nodo lo ponemos como padre del hijo
    void add_Child(Node* hijo) {
        hijo->parent = this;
        children.push_back(hijo);
    }

    // Preorden: guarda la celda al ENTRAR, luego recorre los hijos
    void preorder(vector<string> &lista) {
        lista.push_back(value);
        for (Node* hijo : children) hijo->preorder(lista);
    }

    // Postorden: recorre los hijos y guarda la celda al SALIR (backtracking)
    void postorder(vector<string> &lista) {
        for (Node* hijo : children) hijo->postorder(lista);
        lista.push_back(value);
    }

    // Busca en el arbol el nodo con esas coordenadas
    Node* buscarNodo(pair<int, int> destino) {
        if (coord == destino) return this;
        for (Node* hijo : children) {
            Node* res = hijo->buscarNodo(destino);
            if (res != nullptr) return res;
        }
        return nullptr;
    }
};

// ==============================================================================
// ESTRUCTURA DEL LABERINTO
// ==============================================================================
struct LaberintoData {
    vector<string> matriz;
    pair<int, int> inicio;   // posicion de 'A'
    pair<int, int> meta;     // posicion de 'B'
    int filas;
    int cols;
};

// Orden de exploracion: Arriba, Abajo, Izquierda, Derecha
const int dr[4] = {-1, 1, 0, 0};
const int dc[4] = {0, 0, -1, 1};

// ==============================================================================
// DECLARACION DE FUNCIONES (se construyen debajo del main)
// ==============================================================================
LaberintoData cargarLaberinto(const string &nombreArchivo);
bool esValida(const LaberintoData &lab, int r, int c);
string coordATexto(pair<int, int> p);
Node* dfsConstruirArbol(const LaberintoData &lab, int r, int c, set<pair<int, int>> &visitados);
vector<string> rutaDesdeNodo(Node* nodoMeta);
void ejecutarBFS(const LaberintoData &lab, vector<string> &recorridoBFS, vector<string> &solucionBFS);
void guardarArchivo(const string &nombre, const vector<string> &datos);

// ==============================================================================
// MAIN
// ==============================================================================
int main() {
    cout << "Ingrese el nombre del archivo de entrada (por defecto 'archivo.in'): ";
    string nombreArchivo;
    getline(cin, nombreArchivo);
    if (nombreArchivo.empty()) nombreArchivo = "archivo.in";

    // 1. Cargar el laberinto
    LaberintoData lab = cargarLaberinto(nombreArchivo);

    if (lab.matriz.empty()) {
        cout << "Error: no se encontro o esta vacio el archivo '" << nombreArchivo << "'." << endl;
        return 1;
    }
    if (lab.inicio.first == -1 || lab.meta.first == -1) {
        cout << "Error: el laberinto debe tener una 'A' y una 'B'." << endl;
        return 1;
    }

    // 2. DFS: construir el arbol empezando desde 'A'
    set<pair<int, int>> visitadosDFS;
    Node* raiz = dfsConstruirArbol(lab, lab.inicio.first, lab.inicio.second, visitadosDFS);

    // 3. Preorden, postorden y solucion DFS
    vector<string> dfsPre, dfsPost, solucionDFS;
    if (raiz != nullptr) {
        raiz->preorder(dfsPre);
        raiz->postorder(dfsPost);

        Node* nodoMeta = raiz->buscarNodo(lab.meta);
        solucionDFS = rutaDesdeNodo(nodoMeta);
    }

    // 4. BFS
    vector<string> bfsRecorrido, solucionBFS;
    ejecutarBFS(lab, bfsRecorrido, solucionBFS);

    // 5. Guardar los 5 archivos de salida
    guardarArchivo("recorrido_dfs_preorden.txt", dfsPre);
    guardarArchivo("recorrido_dfs_postorden.txt", dfsPost);
    guardarArchivo("solucion_dfs.txt", solucionDFS);
    guardarArchivo("recorrido_bfs.txt", bfsRecorrido);
    guardarArchivo("solucion_bfs.txt", solucionBFS);

    cout << "Archivos de salida generados con exito:" << endl;
    cout << " 1. recorrido_dfs_preorden.txt" << endl;
    cout << " 2. recorrido_dfs_postorden.txt" << endl;
    cout << " 3. solucion_dfs.txt" << endl;
    cout << " 4. recorrido_bfs.txt" << endl;
    cout << " 5. solucion_bfs.txt" << endl;

    return 0;
}

// ==============================================================================
// DEFINICION DE FUNCIONES
// ==============================================================================

// Lee el archivo: la primera linea son las dimensiones, el resto es el laberinto
LaberintoData cargarLaberinto(const string &nombreArchivo) {
    LaberintoData data;
    data.inicio = {-1, -1};
    data.meta = {-1, -1};
    data.filas = 0;
    data.cols = 0;

    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) return data;

    string linea;
    getline(archivo, linea);  // saltar la linea de dimensiones

    int r = 0;
    while (getline(archivo, linea)) {
        string fila = "";
        for (char ch : linea) {
            if (ch != ' ' && ch != '\r' && ch != '\t') fila += ch;
        }
        if (fila.empty()) continue;

        for (int c = 0; c < (int)fila.size(); c++) {
            if (fila[c] == 'A') data.inicio = {r, c};
            if (fila[c] == 'B') data.meta = {r, c};
        }
        data.matriz.push_back(fila);
        r++;
    }

    data.filas = data.matriz.size();
    data.cols = data.matriz.empty() ? 0 : data.matriz[0].size();
    return data;
}

// Una celda es valida si esta dentro del laberinto y no es pared ('0')
bool esValida(const LaberintoData &lab, int r, int c) {
    if (r < 0 || r >= lab.filas || c < 0 || c >= lab.cols) return false;
    return lab.matriz[r][c] != '0';
}

// Convierte una coordenada a texto: {3,3} -> "(3,3)"
string coordATexto(pair<int, int> p) {
    return "(" + to_string(p.first) + "," + to_string(p.second) + ")";
}

// DFS recursivo que construye el arbol.
// Cada celda visitada es un nodo; sus hijos son las celdas que se descubren
// desde ella, en el orden: Arriba, Abajo, Izquierda, Derecha.
Node* dfsConstruirArbol(const LaberintoData &lab, int r, int c, set<pair<int, int>> &visitados) {
    if (!esValida(lab, r, c) || visitados.count({r, c}) > 0) return nullptr;

    visitados.insert({r, c});
    Node* actual = new Node(r, c);

    for (int i = 0; i < 4; i++) {
        Node* hijo = dfsConstruirArbol(lab, r + dr[i], c + dc[i], visitados);
        if (hijo != nullptr) {
            actual->add_Child(hijo);
        }
    }
    return actual;
}

// Sube desde el nodo de la meta hasta la raiz metiendo cada celda en una PILA.
// Al sacarlas de la pila quedan en orden A -> B.
vector<string> rutaDesdeNodo(Node* nodoMeta) {
    vector<string> ruta;
    stack<string> pila;

    for (Node* n = nodoMeta; n != nullptr; n = n->parent) {
        pila.push(n->value);
    }
    while (!pila.empty()) {
        ruta.push_back(pila.top());
        pila.pop();
    }
    return ruta;
}

// BFS con COLA: recorrido por niveles y ruta mas corta de A a B
void ejecutarBFS(const LaberintoData &lab, vector<string> &recorridoBFS, vector<string> &solucionBFS) {
    queue<pair<int, int>> q;
    set<pair<int, int>> visitados;
    map<pair<int, int>, pair<int, int>> padreMap;  // hijo -> padre
    bool alcanzado = false;

    q.push(lab.inicio);
    visitados.insert(lab.inicio);

    while (!q.empty()) {
        pair<int, int> actual = q.front();
        q.pop();
        recorridoBFS.push_back(coordATexto(actual));

        if (actual == lab.meta) {
            alcanzado = true;
            break;
        }

        for (int i = 0; i < 4; i++) {
            int nr = actual.first + dr[i];
            int nc = actual.second + dc[i];

            if (esValida(lab, nr, nc) && visitados.count({nr, nc}) == 0) {
                visitados.insert({nr, nc});
                padreMap[{nr, nc}] = actual;
                q.push({nr, nc});
            }
        }
    }

    // Reconstruir la ruta de B hacia A con una PILA, para sacarla en orden A -> B
    if (alcanzado) {
        stack<string> pila;
        pair<int, int> curr = lab.meta;
        while (curr != lab.inicio) {
            pila.push(coordATexto(curr));
            curr = padreMap[curr];
        }
        pila.push(coordATexto(lab.inicio));

        while (!pila.empty()) {
            solucionBFS.push_back(pila.top());
            pila.pop();
        }
    }
}

// Guarda los datos en un archivo, separados por " -> "
void guardarArchivo(const string &nombre, const vector<string> &datos) {
    ofstream f(nombre);
    for (size_t i = 0; i < datos.size(); i++) {
        f << datos[i];
        if (i + 1 < datos.size()) f << " -> ";
    }
}