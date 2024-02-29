#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <limits>
#include <random>
#include <queue>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <future>

using namespace std;

int** tab;
chrono::high_resolution_clock::time_point start;
chrono::duration<double, milli> timePassed;
chrono::duration<double, milli> timePassed2;

void loadIniValues(string fileName) {

}

void loadDataFromFile(string fileName, int &numberOfCities) {
    ifstream file;
    file.open(fileName);

    if (file.good()) {
        file >> numberOfCities;

        tab = new int* [numberOfCities];

        for (int i = 0; i < numberOfCities; i++) {
            tab[i] = new int[numberOfCities];
            for (int j = 0; j < numberOfCities; j++) {
                file >> tab[i][j];
            }
        }

        file.close();
    }
    else cout << "Something went wrong with file opening." << endl;
}

vector<int> initializeFirstSolution(int size) { // inicjalizacja początkowego rozwiązania przy pomocy shuffle - losowe rozwiązanie
    vector<int> solution(size);
    for (int i = 0; i < size; i++) {
        solution[i] = i;
    }

    random_device rd;
    mt19937 g(rd());
    shuffle(solution.begin(), solution.end(), g);
    return solution;
}

//struktura przechowująca parę danych koszt-sąsiad
struct Solution {
    int cost;
    vector<int> solution;
};

struct Comp {
    //operator porównania
    bool operator()(const Solution& a, const Solution& b) const {
        return  a.cost > b.cost;
    }
};

int calculateCost(vector<int> solution) {

    int cost = 0;
    for (int i = 0; i < solution.size() - 1; i++) {
        cost += tab[solution[i]][solution[i + 1]];
    }
    cost += tab[solution[solution.size() - 1]][solution[0]];

    return cost;
}

void createNeighborhood(vector<int> currentSolution, priority_queue <Solution, vector<Solution>, Comp>& pq) {
    pq = priority_queue<Solution, vector<Solution>, Comp>();
    Solution s;

    for (int i = 0; i < currentSolution.size(); i++) {
        for (int j = i; j < currentSolution.size(); j++) {
            if (i != j) {
                swap(currentSolution[i], currentSolution[j]);
                s.cost= calculateCost(currentSolution);
                s.solution = currentSolution;
                pq.push(s);
                swap(currentSolution[i], currentSolution[j]);
            }
        }
    }
}

void openIniFile(string fileName, int& iterations, int& maxTabuListSize, vector<string>& files) {
    ifstream iniFile(fileName);
    if (!iniFile) {
        cout << "Nie udało się otworzyć pliku " << fileName << endl;
    }
    else {
        string line;
        while (getline(iniFile, line)) {
            istringstream iss(line);
            string key, value;

            if (getline(iss, key, '=') && getline(iss, value)) {
                if (key == "FileName") {
                    istringstream fileStream(value);
                    string name;
                    while (getline(fileStream, name, ',')) {
                        name.erase(remove_if(name.begin(), name.end(), [](char c) {return isspace(c); }), name.end()); //usuniecie bialych znakow z nazwy
                        files.push_back(name);
                    }
                }
                else if (key == "Iterations")
                    iterations = stoi(value);
                else if (key == "TabuListSize") 
                    maxTabuListSize = stoi(value);  
            }
        }
    }

}

void dotPrinter() {
    int dotWritten = 0;
    while (true) {
        if (dotWritten == 0)
            cout << "Przetwarzanie w toku" << flush;
        cout << "." << flush;
        this_thread::sleep_for(chrono::seconds(1));
        dotWritten++;

        if (dotWritten == 3) {
            system("cls");
            dotWritten = 0;
        }
    }
}

int main() {
    vector<vector<int>> tabuList;
    vector<string> files;
    vector<int> currentSolution, bestSolutionFound;
    int iterations = 0, shuffle = 0, maxTabuListSize, numOfCities = 0, currentSolutionCost, bestSolutionFoundCost, size;
    double sum;
    ostringstream result;

    openIniFile("parametry.ini", iterations, maxTabuListSize, files);

    thread dotThread(dotPrinter);
    int n = 5;

    for (int i = 0; i < files.size(); i++) {

        loadDataFromFile(files[i], numOfCities);
        result << files[i] << "\n";

        if (maxTabuListSize == -1)
            maxTabuListSize = numOfCities / 2;


        currentSolution = initializeFirstSolution(numOfCities);
        currentSolutionCost = calculateCost(currentSolution);
        bestSolutionFoundCost = INT_MAX;
        sum = 0;
        priority_queue <Solution, vector<Solution>, Comp> pq;

        for (int x = 0; x < n; x++) {
            currentSolution = initializeFirstSolution(numOfCities);
            currentSolutionCost = calculateCost(currentSolution);
            start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < iterations; i++) {

                //tworzymy sasiedztwo dla lokalnie najlepszego rozwiazania z użyciem kolejki priorytetowej żeby łatwiej znaleźć najlepsze rozwiązanie w sąsiedztwie
                createNeighborhood(currentSolution, pq);
                size = pq.size();

                for (int j = 0; j < size; j++) {

                    //pobranie najlepszego sąsiada
                    if (pq.size() != 0) {
                        currentSolution = pq.top().solution;
                        currentSolutionCost = pq.top().cost;

                        //jeśli sąsiad jest w tabuList bierzemy następnego najlepszego 
                        if (find(tabuList.begin(), tabuList.end(), currentSolution) != tabuList.end()) {
                            pq.pop();
                        }
                        else {

                            if (currentSolutionCost < bestSolutionFoundCost) {
                                bestSolutionFound = currentSolution;
                                bestSolutionFoundCost = currentSolutionCost;
                                shuffle = 0;
                            }
                            else {
                                shuffle++;
                            }

                            tabuList.insert(tabuList.begin(), currentSolution);

                            if (tabuList.size() > maxTabuListSize) {
                                tabuList.pop_back();
                            }
                            break;
                        }
                    }
                    
                }
                if (shuffle > iterations / 5) {
                    shuffle = 0;
                    currentSolution = initializeFirstSolution(numOfCities);
                    currentSolutionCost = calculateCost(currentSolution);
                    while (pq.size() != 0) 
                        pq.pop();
                }
            }
            sum += bestSolutionFoundCost;
            result << (x + 1) << ". Wynik: " << bestSolutionFoundCost << ". Droga: ";
            for (int i = 0; i < bestSolutionFound.size(); i++) {
                result << bestSolutionFound[i] << " ";
            }
            result << " Czas: ";                    
            bestSolutionFound.clear();
            bestSolutionFoundCost = INT_MAX;
            shuffle = 0;
            timePassed = std::chrono::high_resolution_clock::now() - start;
            timePassed2 += timePassed;
            result << ceil((timePassed.count()) * 1000) / 1000 << "ms\n"; 
            
        }
        
        result << "Average cost: " << sum / n << endl;
        result << "Average time: " << ceil((timePassed2.count()/n) * 1000) / 1000 << " ms (iterations = " << iterations <<")\n";
    }    
    dotThread.detach();
    system("cls");
    cout << result.str();

    
    return 0;
}