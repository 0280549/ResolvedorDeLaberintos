#include <iostream>
#include <unordered_map>
#include <string>
#include <fstream>
#include <vector>

using namespace std;
int main(){
	int cols;
	int rows;
	vector<string> maze;
	string filename;
	cout << "Enter the filename: ";
	cin >> filename;
	ifstream file(filename);
	file >> rows >> cols;
	while(file){
		string line;
		getline(file, line);
		if(line.length() > 0){
			maze.push_back(line);
		}
	}

	cout << "Rows: " << rows << endl;
	cout << "Cols: " << cols << endl;
	for(int i = 0; i < maze.size(); i++){
		cout << maze[i] << endl;
	}



}