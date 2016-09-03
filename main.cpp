#include <QApplication>
#include <QLabel>
#include <QPicture>
#include <QPainter>

#include <fstream>
#include <string>
#include <sstream>

#include <QtDebug>
#include <time.h>

using namespace std;


// Created these for easier sorting purposes
struct proteinNode {
    string proteinDirection;
    int fitness;
};
// Test for order
struct ascending {
    bool operator()(proteinNode const &a, proteinNode const &b) {
        return a.fitness < b.fitness;
    }
};




// Constructors
vector<proteinNode> generateInitialPop(int, int, int);
string createRandomSequence(int, int);
proteinNode grabParent(vector<proteinNode>, int);
proteinNode crossover(proteinNode, proteinNode, int, int);
void displayProtein(QPicture, int);

QPicture drawProtein(string, string, int, int);
bool collisionDetection(string, int);
vector<vector<char>> getDirectionalSequenceMap(string, string, int);
int getFitnessRating(string, string, int);
vector<string> split(string, char);






int main(int argc, char *argv[])
{
    // START: User options

    //srand(time(NULL));

    // Change as per location. Will not read relatively to the location of the program for some reason
    string filename = "C:\\test\\Input.txt";

    // Fitness level can be from zero to 1024
    // Max fitness level from input file
    const int maxFitnessLimit = 1024;

    // proteinSequence = h=Hydrophobic(special/red) p=Hydrophilic(black)
    string proteinSequence  = "hphpphhphpphphhpphph";

    // Directions the protein travels from each node
    // 1=North 2=East 3=South 4=West, 0=Nowhere(end)
    string proteinDirection = "21412141214121412140";

    // For spacing between points
    int pixelSpacing = 25;

    // Window size
    int windowWidth = 800;
    int windowHeight = 600;

    // Genetic options:
    int popNum = 100;

    // Percentage of elite, calculated into the exact number based on popNum
    int elitePercentage = 5;
    int numElite = (elitePercentage/100) * popNum;

    // First half will be eligible, if second half is picked it rolls again for the second half as the first half
    // It continues trickling down until it rolls a random number in the first half
    int percentChanceParent = 10;


    // END: User options


    // Each index matches each other for simplicity
    vector<string> testSequence;
    vector<int> testFitness;


    // Setup QT's main window and label
    QApplication a(argc, argv);
    QLabel l;

    // Show window and loading message in console
    a.processEvents();
    qDebug("Loading...");

    l.setAlignment(Qt::AlignCenter);
    l.setFixedSize(windowWidth,windowHeight);


    // Setup input file stream
    ifstream readFile;
    readFile.open(filename.c_str());
    string lineInput;

    int numTestCases;

    // Read file line by line and parse it as it is read
    if(readFile.is_open()) {
        // Setup loop for grabbing test cases
        getline(readFile, lineInput);
        vector<string> splitString = split(lineInput, ' ');

        numTestCases = stoi(splitString[2]);

        // Add the sequence and maxFitnessLimit to corresponding vectors
        for(int i=0;i<numTestCases;i++) {
            getline(readFile, lineInput);
            splitString = split(lineInput, ' ');
            if(splitString[0] == "Seq") {
                testSequence.push_back(splitString[2]);
                //qDebug(splitString[2].c_str());
            }

            getline(readFile, lineInput);
            splitString = split(lineInput, ' ');
            if(splitString[0] == "Fitness") {
                testFitness.push_back(stoi(splitString[2]));
                //qDebug(splitString[2].c_str());
            }
        }
    } else {
        string error = "Error opening file: " + filename + "\n" + "ERROR: " + strerror(errno);
        qDebug(error.c_str());
        return 1;
    }





    // Does the genetic algorithm for every test case in input file
    for(int case_i=0;case_i<numTestCases;case_i++) {

        // Get current sequence and target fitness
        string currSequence = testSequence[case_i];
        int currSize = currSequence.size();
        int targetFitness = testFitness[case_i];

        // Generate initial population
        vector<proteinNode> population;
        population = generateInitialPop(popNum, currSize, maxFitnessLimit);


        // Keeps track of the number of generations, starts at 0 for easy iteration (first generation will be 1)
        int generationNum = 0;

        int currentFitness = 0;
        while(currentFitness > targetFitness) {

            generationNum++;

            // Generate the fitness rating for each member of the population
            for(int i=0;i<popNum;i++) {
                population[i].fitness = getFitnessRating(proteinSequence, population[i].proteinDirection, maxFitnessLimit);
            }

            // Sort the vector based on the fitness rating
            sort(population.begin(), population.end(), ascending());

            // Create second population vector
            vector<proteinNode> nextPopulation;

            while(nextPopulation.size() < popNum) {
                proteinNode parent1 = grabParent(population, percentChanceParent);
                proteinNode parent2 = grabParent(population, percentChanceParent);
                // Makes sure the second parent isn't the same
                while(parent1.proteinDirection == parent2.proteinDirection) {
                    parent2 = grabParent(population, percentChanceParent);
                }

                proteinNode child = crossover(parent1, parent2, percentChanceParent, maxFitnessLimit);
                while(child.proteinDirection == "nope") {
                    // Choose new parents
                    parent1 = grabParent(population, percentChanceParent);
                    parent2 = grabParent(population, percentChanceParent);
                    // Make sure the second parent isn't the same
                    while(parent1.proteinDirection == parent2.proteinDirection) {
                        parent2 = grabParent(population, percentChanceParent);
                    }

                    child = crossover(parent1, parent2, percentChanceParent, maxFitnessLimit);
                }

                nextPopulation.push_back(child);
            }

            for(int i=0;i<popNum;i++) {
                nextPopulation[i].fitness = getFitnessRating(proteinSequence, nextPopulation[i].proteinDirection, maxFitnessLimit);
            }

            currentFitness = nextPopulation[0].fitness;
            population = nextPopulation;

            string generation = "----- Generation: " + to_string(generationNum) + " -----";
            string currentFitString = "Fitness:    " + to_string(currentFitness);
            string currentDirections = "Directions: " + population[0].proteinDirection;

            qDebug(generation.c_str());
            qDebug(currentFitString.c_str());
            qDebug(currentDirections.c_str());
            qDebug("");

            // Display best fit in generation
            QPicture pi = drawProtein(proteinSequence, population[0].proteinDirection, maxFitnessLimit, pixelSpacing);
            int fitness = population[0].fitness;

            string fitText = "Fitness: " + to_string(fitness);

            // Setup child label for displaying the fitness level
            QLabel r(&l);
            QFont f("Arial", 16, QFont::Bold);
            r.setMargin(10);
            r.setFont(f);
            r.setAlignment(Qt::AlignTop);
            r.setText(QString::fromStdString(fitText));

            // Draw parent QLabel, containing the image and fitness sub-QLabel
            l.setPicture(pi);
            l.show();

            a.processEvents();
        }
    }

    return 1;
}



vector<proteinNode> generateInitialPop(int amount, int length, int maxFitnessLimit) {
    vector<proteinNode> population;

    for(int i=0;i<amount;i++) {
        proteinNode tempNode;
        population.push_back(tempNode);
        string sequence = createRandomSequence(length, maxFitnessLimit);
        population[i].proteinDirection = sequence;
    }
    return population;
}


// Grabs a parent using a weighted selection method
proteinNode grabParent(vector<proteinNode> population, int percentChanceParent) {
    int popToGrab = -1;
    int toAdd = 0;
    int tempChance = percentChanceParent * 2;

    while(popToGrab == -1) {
        int randomChance = rand() % tempChance;
        if(randomChance < tempChance/2) {
            popToGrab = randomChance;
        } else {
            if(toAdd >= population.size() - tempChance) {
                popToGrab = randomChance + toAdd;
            } else {
                toAdd += percentChanceParent;
            }
        }
    }

    return population[popToGrab];
}


// Crosses 2 proteins over, if they can be crossed. Otherwise, returns "nope" in the proteinDirection
proteinNode crossover(proteinNode parent1, proteinNode parent2, int numToTry, int maxFitnessLimit) {
    int sizeParents = parent1.proteinDirection.size();
    proteinNode child;

    for(int i=0;i<numToTry;i++) {
        int randomIndex = rand() % sizeParents;

        for(int i=randomIndex;i<sizeParents;i++) {
            parent1.proteinDirection[i] = parent2.proteinDirection[i];
        }

        if(!collisionDetection(parent1.proteinDirection, maxFitnessLimit)) {
            return parent1;
        }
    }

    child.proteinDirection = "nope";
    return child;
}

















// Generate random valid structure
string createRandomSequence(int length, int maxFitnessLimit) {
    string randomSequence;

    // While the directional sequence isn't valid, keep generating until a valid one is produced
    bool valid = false;
    while(!valid) {
        vector<vector<int>> collisionTestMap(maxFitnessLimit*2+3, vector<int>(maxFitnessLimit*2+3, 0));
        int currX = maxFitnessLimit + 1;
        int currY = maxFitnessLimit + 1;

        // Mark starting point
        collisionTestMap[currX][currY] = 1;


        for(int i=0;i<length;i++) {
            int currentDirection;
            //qDebug(to_string(i).c_str());
            // Check if last, if so use a 0 instead of 1-4
            if(i == length-1) {
                randomSequence.append(to_string(0));
            } else {
                // 1,2,3,4
                currentDirection = (rand() % 4) + 1;

                // Check if collision & randomly go clock or counter clockwise for directions
                // 0=Clockwise, 1=Counter
                int searchDirection = rand() % 2;
                for(int j=0;j<3;j++) {
                    if(currentDirection == 1 && collisionTestMap[currX][currY-1] != 1) {
                        currY--;
                        break;
                    }
                    else if(currentDirection == 2 && collisionTestMap[currX+1][currY] != 1) {
                        currX++;
                        break;
                    }
                    else if(currentDirection == 3 && collisionTestMap[currX][currY+1] != 1) {
                        currY++;
                        break;
                    }
                    else if(currentDirection == 4 && collisionTestMap[currX-1][currY] != 1) {
                        currX--;
                        break;
                    } else {
                        if(searchDirection == 0) {
                            currentDirection++;
                        } else {
                            currentDirection--;
                        }

                        if(currentDirection > 4) {
                            currentDirection = 1;
                        } else if(currentDirection < 1) {
                            currentDirection = 4;
                        }
                    }
                }
                // Mark cell and append string
                collisionTestMap[currX][currY] = 1;
                randomSequence.append(to_string(currentDirection));
            }
        }
        if(!collisionDetection(randomSequence, maxFitnessLimit)) {
            valid = true;
        } else {
            randomSequence = "";
        }
    }

    return randomSequence;
}


// Fitness function, reads in vector<vector<char>> and returns a fitness rating integer
int getFitnessRating(string proteinSequence, string proteinDirection, int maxFitnessLimit) {

    vector<vector<int>> collisionTestMap(maxFitnessLimit*2+3, vector<int>(maxFitnessLimit*2+3, 0));

    int prevX;
    int prevY;

    int currX = maxFitnessLimit + 1;
    int currY = maxFitnessLimit + 1;

    int nextX = currX;
    int nextY = currY;

    // Keeps track of fitness level
    int fitness = 0;

    // Generate directional sequence map
    vector<vector<char>> directionalSequenceMap = getDirectionalSequenceMap(proteinSequence, proteinDirection, maxFitnessLimit);

    for(int i=0;i<proteinDirection.size();i++) {

        char currentDirection = proteinDirection[i];
        char currentType = directionalSequenceMap[currX][currY];

        // Find the next coordinates
        if(currentDirection == '1') {
            nextY = currY - 1;
        }
        else if(currentDirection == '2') {
            nextX = currX + 1;
        }
        else if(currentDirection == '3') {
            nextY = currY + 1;
        }
        else if(currentDirection == '4') {
            nextX = currX - 1;
        }

        // Take into account last element, which will not have a nextX or nextY
        if(i == proteinDirection.size()-1) {
            nextX = -1;
            nextY = -1;
        }

        // Check all 4 neighbors in each direction, but only if they are not the
        // previous or next coordinates AND they have not been visited before.
        if(currentType == 'h') {
            if(!(currX == prevX && currY-1 == prevY) && !(currX == nextX && currY-1 == nextY)) {
                if(directionalSequenceMap[currX][currY-1] == 'h' && collisionTestMap[currX][currY-1] != 1) {
                    fitness--;
                }
            }
            if(!(currX+1 == prevX && currY == prevY) && !(currX+1 == nextX && currY == nextY)) {
                if(directionalSequenceMap[currX+1][currY] == 'h' && collisionTestMap[currX+1][currY] != 1) {
                    fitness--;
                }
            }
            if(!(currX == prevX && currY+1 == prevY) && !(currX == nextX && currY+1 == nextY)) {
                if(directionalSequenceMap[currX][currY+1] == 'h' && collisionTestMap[currX][currY+1] != 1) {
                    fitness--;
                }
            }
            if(!(currX-1 == prevX && currY == prevY) && !(currX-1 == nextX && currY == nextY)) {
                if(directionalSequenceMap[currX-1][currY] == 'h' && collisionTestMap[currX-1][currY] != 1) {
                    fitness--;
                }
            }
        }


        // Mark cell as visited so it is not recounted later
        collisionTestMap[currX][currY] = 1;

        prevX = currX;
        prevY = currY;

        currX = nextX;
        currY = nextY;
    }

    return fitness;
}



// Calculate and return map containing laid out protein shape with each cell marked with a protein type as per directional map
vector<vector<char>> getDirectionalSequenceMap(string proteinSequence, string proteinDirection, int maxFitnessLimit) {
    // Initialized to zero, used for detecting collisions when combining proteins.
    vector<vector<char>> proteinSequence2D(maxFitnessLimit*2+3, vector<char>(maxFitnessLimit*2+3));
    int currX = maxFitnessLimit + 1;
    int currY = maxFitnessLimit + 1;

    for(int i=0;i<proteinDirection.size();i++) {

        proteinSequence2D[currX][currY] = proteinSequence[i];

        char currentDirection = proteinDirection[i];

        if(currentDirection == '1') {
            currY -= 1;
        }
        else if(currentDirection == '2') {
            currX += 1;
        }
        else if(currentDirection == '3') {
            currY += 1;
        }
        else if(currentDirection == '4') {
            currX -= 1;
        }
    }

    return proteinSequence2D;
}


// Detects if a protein's path intersects itself. If it does, return true.
bool collisionDetection(string proteinDirection, int maxFitnessLimit) {
    // Initialized to zero, used for detecting collisions when combining proteins.
    vector<vector<int>> collisionTestMap(maxFitnessLimit*2+3, vector<int>(maxFitnessLimit*2+3, 0));
    int currX = maxFitnessLimit + 1;
    int currY = maxFitnessLimit + 1;

    // Mark starting point
    collisionTestMap[currX][currY] = 1;

    for(int i=0;i<proteinDirection.size();i++) {

        char currentDirection = proteinDirection[i];

        if(currentDirection == '1') {
            currY -= 1;
        }
        else if(currentDirection == '2') {
            currX += 1;
        }
        else if(currentDirection == '3') {
            currY += 1;
        }
        else if(currentDirection == '4') {
            currX -= 1;
        }

        // Check if cell is marked. If it is collision return true, if not mark cell
        if(collisionTestMap[currX][currY] == 1 && currentDirection != '0') {
            return true;
        } else {
            collisionTestMap[currX][currY] = 1;
        }
    }

    return false;
}



// Draws and returns a QPicture based on the input sequence and direction inputs
// proteinSequence = h=Hydrophobic(special/red) p=Hydrophilic(black)
QPicture drawProtein(string proteinSequence, string proteinDirection, int maxFitnessLimit, int pixelSpacing) {
    QPicture pi;
    QPainter p(&pi);

    p.setRenderHint(QPainter::Antialiasing);

    vector<vector<int>> collisionTestMap(maxFitnessLimit*2+3, vector<int>(maxFitnessLimit*2+3, 0));

    // These are for reading the directional sequence map for the dotted links
    int currXmap = maxFitnessLimit + 1;
    int currYmap = maxFitnessLimit + 1;
    int nextXmap;
    int nextYmap;

    int currX = maxFitnessLimit * pixelSpacing;
    int currY = maxFitnessLimit * pixelSpacing;
    int nextX;
    int nextY;

    // Get directional sequence map
    vector<vector<char>> directionalSequenceMap = getDirectionalSequenceMap(proteinSequence, proteinDirection, maxFitnessLimit);

    char currentType;
    char currentDirection;

    for(int i=0;i<proteinSequence.size();i++) {
        currentType = proteinSequence[i];
        currentDirection = proteinDirection[i];


        // First draw the line to the next destination
        p.setPen(QPen(Qt::black, 3, Qt::SolidLine, Qt::FlatCap, Qt::BevelJoin));
        if(currentDirection == '1') {
            nextX = currX;
            nextY = currY - pixelSpacing;

            nextXmap = currXmap;
            nextYmap = currYmap - 1;

            p.drawLine(currX, currY, nextX, nextY);
        }
        else if(currentDirection == '2') {
            nextX = currX + pixelSpacing;
            nextY = currY;

            nextXmap = currXmap + 1;
            nextYmap = currYmap;

            p.drawLine(currX, currY, nextX, nextY);
        }
        else if(currentDirection == '3') {
            nextX = currX;
            nextY = currY + pixelSpacing;

            nextXmap = currXmap;
            nextYmap = currYmap + 1;

            p.drawLine(currX, currY, nextX, nextY);
        }
        else if(currentDirection == '4') {
            nextX = currX - pixelSpacing;
            nextY = currY;

            nextXmap = currXmap - 1;
            nextYmap = currYmap;

            p.drawLine(currX, currY, nextX, nextY);
        }

        // Next draw dotted lines for fitness connections
        // (Do not worry about duplicates from lines since they will be overlapped by the solid black lines)
        p.setPen(QPen(Qt::black, 1, Qt::DotLine, Qt::FlatCap, Qt::BevelJoin));
        if(currentType == 'h') {
            if(directionalSequenceMap[currXmap][currYmap-1] == 'h' && collisionTestMap[currXmap][currYmap-1] != 1) {
                p.drawLine(currX, currY, currX, currY-pixelSpacing);
            }
            if(directionalSequenceMap[currXmap+1][currYmap] == 'h' && collisionTestMap[currXmap+1][currYmap] != 1) {
                p.drawLine(currX, currY, currX+pixelSpacing, currY);
            }
            if(directionalSequenceMap[currXmap][currYmap+1] == 'h' && collisionTestMap[currXmap][currYmap+1] != 1) {
                p.drawLine(currX, currY, currX, currY+pixelSpacing);
            }
            if(directionalSequenceMap[currXmap-1][currYmap] == 'h' && collisionTestMap[currXmap-1][currYmap] != 1) {
                p.drawLine(currX, currY, currX-pixelSpacing, currY);
            }
        }



        // Finally draw the dot for the type of amino acid on top of the lines
        if(currentType == 'h') {
            p.setPen(QPen(Qt::red, 12, Qt::SolidLine, Qt::RoundCap));
            p.drawPoint(currX, currY);
        } else {
            p.setPen(QPen(Qt::black, 12, Qt::SolidLine, Qt::RoundCap));
            p.drawPoint(currX, currY);
        }

        collisionTestMap[currXmap][currYmap] = 1;

        currXmap = nextXmap;
        currYmap = nextYmap;

        currX = nextX;
        currY = nextY;

    }
    if(currentType == 'h') {
        p.setPen(QPen(Qt::red, 12, Qt::SolidLine, Qt::RoundCap));
        p.drawPoint(currX, currY);
    } else {
        p.setPen(QPen(Qt::black, 12, Qt::SolidLine, Qt::RoundCap));
        p.drawPoint(currX, currY);
    }


    p.end();

    return pi;
}






// HELPER FUNCTIONS

// Splits a string by delimiter
vector<string> split(string str, char delimiter) {
    vector<string> strings;
    istringstream f(str);
    string s;

    while (getline(f, s, delimiter)) {
        strings.push_back(s);
    }

    return strings;
}


