#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <set>
#include <map>
#include <queue>
#include <stack>

using namespace std;

// Construccion del arbol de DFS////////////

// Hacemos la clase nodo como lo aprendido en clase
class Node {
public:
    string value;            // La coordenada la guardamos como string: "(fila,columna)"
    pair<int, int> coord;    // Par {fila, columna}
    Node* parent;
    vector<Node*> children;

    Node(int x, int y);
    void add_Child(Node* child);
    void preorder(vector<string> &out_List);
    void postorder(vector<string> &out_List);
    Node* find_Node(pair<int, int> target);
};

// Como organizamos el laberinto en memoria: una estructura que contiene la matriz, las posiciones de inicio y salida, y las dimensiones
struct Maze_Data {
    vector<string> matrix;
    pair<int, int> start_A;   // posicion de 'A'
    pair<int, int> exit_B;    // posicion de 'B'
    int rows;
    int cols;
};

// Solo se puede ir: Arriba, Abajo, Izquierda, Derecha
const int runner_Row[4] = {-1, 1, 0, 0};
const int runner_Col[4] = {0, 0, -1, 1};

//solo declaramos funciones, abajo se construyen
Maze_Data load_Maze(const string &file_Name);
bool is_Valid(const Maze_Data &maze, int r, int c);
string coord_To_Text(pair<int, int> p);
Node* dfs_Build_Tree(const Maze_Data &maze, int r, int c, set<pair<int, int>> &visited);
vector<string> path_From_Node(Node* exit_Node);
void make_BFS(const Maze_Data &maze, vector<string> &bfs_Route, vector<string> &bfs_Solution);
void save_File(const string &name, const vector<string> &data_List);

int main() {
    cout << "Enter the input file name (default 'archivo.in'): ";
    string file_Name;
    getline(cin, file_Name);
    if (file_Name.empty()) file_Name = "archivo.in";

    // 1. Cargar el laberinto
    Maze_Data maze = load_Maze(file_Name);

    if (maze.matrix.empty()) {
        cout << "Error: the file '" << file_Name << "' was not found or is empty." << endl;
        return 1;
    }
    if (maze.start_A.first == -1 || maze.exit_B.first == -1) {
        cout << "Error: the maze must have an 'A' and a 'B'." << endl;
        return 1;
    }

    // 2. DFS: construir el arbol empezando desde 'A'
    set<pair<int, int>> dfs_Visited;
    Node* root = dfs_Build_Tree(maze, maze.start_A.first, maze.start_A.second, dfs_Visited);

    // 3. Preorden, postorden y solucion DFS
    vector<string> dfs_Pre, dfs_Post, dfs_Solution;
    if (root != nullptr) {
        root->preorder(dfs_Pre);
        root->postorder(dfs_Post);

        Node* exit_Node = root->find_Node(maze.exit_B);
        dfs_Solution = path_From_Node(exit_Node);
    }

    // 4. BFS
    vector<string> bfs_Route, bfs_Solution;
    make_BFS(maze, bfs_Route, bfs_Solution);

    // 5. Guardar los 5 archivos de salida
    save_File("dfs_preorder_route.txt", dfs_Pre);
    save_File("dfs_postorder_route.txt", dfs_Post);
    save_File("dfs_solution.txt", dfs_Solution);
    save_File("bfs_route.txt", bfs_Route);
    save_File("bfs_solution.txt", bfs_Solution);

    cout << "Output files generated successfully:" << endl;
    cout << " 1. dfs_preorder_route.txt" << endl;
    cout << " 2. dfs_postorder_route.txt" << endl;
    cout << " 3. dfs_solution.txt" << endl;
    cout << " 4. bfs_route.txt" << endl;
    cout << " 5. bfs_solution.txt" << endl;

    return 0;
}

//////FUNCIONES DE NODO///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Constructor: guarda la coordenada y la convierte a texto "(x,y)".
// El padre empieza en nullptr porque al crear el nodo todavia no tiene padre.
Node::Node(int x, int y) {
    coord = {x, y};
    value = "(" + to_string(x) + "," + to_string(y) + ")";
    parent = nullptr;
}

// Agrega un hijo y pone a este nodo como el padre del hijo
void Node::add_Child(Node* child) {
    child->parent = this;
    children.push_back(child);
}

// Preorden: guarda la coordenada al ENTRAR al nodo, luego recorre los hijos
void Node::preorder(vector<string> &out_List) {
    out_List.push_back(value);
    for (Node* child : children) child->preorder(out_List);
}

// Postorden: recorre los hijos y guarda la coordenada al SALIR
void Node::postorder(vector<string> &out_List) {
    for (Node* child : children) child->postorder(out_List);
    out_List.push_back(value);
}

// Busca en el arbol el nodo con esas coordenadas pedidas. Devuelve un puntero al nodo si lo encuentra, o nullptr si no.
Node* Node::find_Node(pair<int, int> target) {
    if (coord == target) return this;
    for (Node* child : children) {
        Node* result = child->find_Node(target);
        if (result != nullptr) return result;
    }
    return nullptr;
}

////////FUNCIONES DE LABERINTO/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Lee el archivo: la primera linea son las dimensiones, el resto es el laberinto
Maze_Data load_Maze(const string &file_Name) {
    Maze_Data data;
    data.start_A = {-1, -1}; //las inicializamos en -1 porque todavía no se encuentran, después se van a actualizar cuando se encuentren en la matriz
    data.exit_B = {-1, -1};
    data.rows = 0;
    data.cols = 0;

    ifstream file(file_Name);
    if (!file.is_open()) return data; // si no se puede abrir el archivo, devolvemos un laberinto vacío para no causar errores

    string line;
    getline(file, line);  // saltar la linea de dimensiones

    int r = 0;
    while (getline(file, line)) {
        string row_Text = "";
        for (char ch : line) { //leemos todas las lineas y vamos guardando en row_Text solo los caracteres que no sean espacio, retorno de carro o tabulador
            if (ch != ' ' && ch != '\r' && ch != '\t') row_Text += ch; //windows agrega un retorno de carro al final de cada linea, por eso lo quitamos. Es invisible a la vista,
            //pero si no lo quitamos, la matriz queda con un caracter de más en cada fila y eso causa errores al recorrerla
        }
        if (row_Text.empty()) continue;

        //buscamos dónde están A y B en la fila actual, y guardamos sus coordenadas en data.start_A y data.exit_B
        for (int c = 0; c < (int)row_Text.size(); c++) { //c= numero de columna, r=numero de fila. (int)= necesitamos cambiarlo a int porque size() devuelve un size_t y eso causa errores al comparar con int
            if (row_Text[c] == 'A') data.start_A = {r, c};
            if (row_Text[c] == 'B') data.exit_B = {r, c};
        }
        data.matrix.push_back(row_Text);
        r++;
    }

    //se guardan las dimensiones del laberinto pero si la matriz está vacía, cols=0 para evitar errores al recorrerla porque al acceder a matrix[0] no existiría
    data.rows = data.matrix.size();
    data.cols = data.matrix.empty() ? 0 : data.matrix[0].size();
    return data;
}
//para no hacer nuestras funciones largas, usamos is_Valid() para chequear si una celda es valida antes de recorrerla
// Una celda es valida si esta dentro del laberinto y no es pared ('0')
bool is_Valid(const Maze_Data &maze, int r, int c) {
    if (r < 0 || r >= maze.rows || c < 0 || c >= maze.cols) return false;
    return maze.matrix[r][c] != '0';
}

// Convierte una coordenada a texto: {3,3} -> "(3,3)"
// Esto es útil para guardar las rutas en los archivos de salida
//es para BFS mas que nada porque en DFS ya tenemos el nodo y podemos acceder a su value, pero en BFS solo tenemos la coordenada y trabajamos con pairs
string coord_To_Text(pair<int, int> p) {
    return "(" + to_string(p.first) + "," + to_string(p.second) + ")";
}

// DFS recursivo que construye el arbol.
// Cada punto visitado es un nodo; sus hijos son los puntos que se descubren
// desde ahí, en el orden: Arriba, Abajo, Izquierda, Derecha.
//si el punto es pared, esta fuera del laberinto o ya fue visitado, devuelve nullptr
//usando recursion
Node* dfs_Build_Tree(const Maze_Data &maze, int r, int c, set<pair<int, int>> &visited) {
    if (!is_Valid(maze, r, c) || visited.count({r, c}) > 0) return nullptr;

    visited.insert({r, c});
    Node* current = new Node(r, c); //creamos un nodo para la celda actual

    for (int i = 0; i < 4; i++) {
        Node* child = dfs_Build_Tree(maze, r + runner_Row[i], c + runner_Col[i], visited);
        if (child != nullptr) {
            current->add_Child(child);
        }
    }
    return current;
}

// Sube desde el nodo de la salida (hoja) hasta la raiz metiendo cada punto en una PILA- como enseñó Alcaraz
// Al sacarlas de la pila quedan en orden A -> B.
vector<string> path_From_Node(Node* exit_Node) {
    vector<string> path;
    stack<string> path_Stack;

    for (Node* n = exit_Node; n != nullptr; n = n->parent) {
        path_Stack.push(n->value);
    }
    //si la salida no está en el arbol, el vector devuelto queda vacio
    while (!path_Stack.empty()) {
        path.push_back(path_Stack.top());
        path_Stack.pop();
    }
    return path;
}

// BFS con COLA: recorrido por niveles y ruta mas corta de A a B
void make_BFS(const Maze_Data &maze, vector<string> &bfs_Route, vector<string> &bfs_Solution) {
    queue<pair<int, int>> q; //guarda los puntos pendientes de visitar, en orden de llegada
    set<pair<int, int>> visited; //evita visitar un punto mas de una vez
    map<pair<int, int>, pair<int, int>> parent_Map;  // hijo -> padre
    bool reached = false; //ahorita ponemos una flag para saber si llegamos a EXIT B

    q.push(maze.start_A); //A en cola
    visited.insert(maze.start_A);//A en visitados

    while (!q.empty()) {
        pair<int, int> current = q.front();
        q.pop();
        bfs_Route.push_back(coord_To_Text(current)); //mientras haya puntos en la cola, los sacamos y los guardamos en el recorrido

        if (current == maze.exit_B) { //si el punto actual es B, terminamos yeiiii
            reached = true;
            break;
        }

        for (int i = 0; i < 4; i++) {//calculamos los puntos de los cuatro vecinos
            int new_Row = current.first + runner_Row[i];
            int new_Col = current.second + runner_Col[i];

            //si el vecino está dentro del laberinto y no es pared && no fue visitado, lo agregamos a la cola y lo marcamos como visitado
            if (is_Valid(maze, new_Row, new_Col) && visited.count({new_Row, new_Col}) == 0) {//usar llaves es lo mismo que hacer un make_pair, pero mas corto
                visited.insert({new_Row, new_Col});
                parent_Map[{new_Row, new_Col}] = current; //guardamos de donde venimos
                q.push({new_Row, new_Col});
            }
        }
    }

    // Reconstruir la ruta de B hacia A con una PILA, para sacarla en orden A -> B
    if (reached) {
        stack<string> path_Stack;
        pair<int, int> cell = maze.exit_B;
        while (cell != maze.start_A) {
            path_Stack.push(coord_To_Text(cell));
            cell = parent_Map[cell];
        }
        path_Stack.push(coord_To_Text(maze.start_A));

        while (!path_Stack.empty()) {
            bfs_Solution.push_back(path_Stack.top());
            path_Stack.pop();
        }
    }
}

// Guarda los datos en un archivo, separados por " -> "
void save_File(const string &name, const vector<string> &data_List) {
    ofstream out(name);
    for (size_t i = 0; i < data_List.size(); i++) {
        out << data_List[i];
        if (i + 1 < data_List.size()) out << " -> ";
    }
}