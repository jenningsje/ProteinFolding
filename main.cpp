#include <QApplication>
#include <QLabel>
#include <QPicture>
#include <QPainter>
#include <QDir>

#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>

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
string mutate(string, int, int);
vector<proteinNode> generateInitialPop(int, int, int);
proteinNode grabParent(vector<proteinNode>, int); // For weighted selection
proteinNode crossover(proteinNode, proteinNode, int, int, string);
string createRandomSequence(int, int);
int getFitnessRating(string, string, int);

vector<vector<char>> getDirectionalSequenceMap(string, string, int);
bool collisionDetection(string, int);

QPicture drawProtein(string, string, int, int);

vector<string> split(string, char);






int main(int argc, char *argv[])
{

    // START: User options

    srand(time(NULL));

    // Contains all the user-modifiable options for the algorithm
    string optionsFilename = "Options.txt";

    // Change as per location. Will not read relatively to the location of the program for some reason
    string filename = "Input.txt";

    // Fitness level can be from zero to 1024
    // Max fitness level from input file
    int maxFitnessLimit = 1024;

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
    int popNum = 200;

    // Percentage of elite, calculated into the exact number based on popNum
    int elitePercentage = 5;
    int numElite = (elitePercentage/100.0) * popNum;

    // NOTE: Program calls for this option to be turned off
    // More fun to visualize by choosing one of the elites to be displayed
    // instead of just the most fit.
    int drawRand = 0;
    // Draws one of the top X percentage randomly after each generation (more fun to look at)
    int drawPercentage = 10;

    // NOTE: Controls 2 things: Number of times to attempt a crossover AND mutation before failure
    int numToTry = 5;

    // User input for percentage to mutate, calculates the number
    int mutatePercentage = 50;
    int numMutate = (mutatePercentage/100.0) * popNum;

    // Calculates the number to crossover for a hard limit to leave room for random generations
    int crossoverPercentage = 70;
    int numCrossover = (crossoverPercentage/100.0) * popNum;

    // APOCALYPSE Options
    // Apocalypse clears the population if the fitness hasn't gotten better after the repeatTrigger's amount
    int apocalypse = 0;
    int apocRepeatTrigger = 200;
    int apocCounter = 0;
    int numApoc = 0;
    int numSurvivors = 0;
    int apocLastFitness;

    // Keeps track of progress
    int numCompleted = 0;

    // Deduplication on the population is done every x generations
    int checkForDupeInterval = 500;

    // END: User options



    // Each index matches each other for simplicity
    vector<string> testSequence;
    vector<int> testFitness;


    // Setup QT's main window and label
    QApplication a(argc, argv);
    QLabel l;

    l.setAlignment(Qt::AlignCenter);
    l.setFixedSize(windowWidth,windowHeight);

    // Show window and loading message in console
    string loadText = "Loading...";

    // Setup child label for displaying the fitness level
    QLabel x(&l);
    QFont f("Arial", 16, QFont::Bold);
    x.setMargin(10);
    x.setFont(f);
    x.setAlignment(Qt::AlignTop);
    x.setText(QString::fromStdString(loadText));
    x.show();

    // Draw parent QLabel, containing the image and fitness sub-QLabel
    l.show();
    // Update window
    a.processEvents();

    // Otherwise the Loading... message will not go away
    x.hide();


    // Setup options input file
    ifstream optionsFile;
    optionsFile.open(optionsFilename.c_str());
    string lineInput;

    // Read file line by line and parse it as it is read
    if(optionsFile.is_open()) {
        // Read in a line and save the variable's value accordingly
        while(getline(optionsFile,lineInput)) {
            vector<string> splitString = split(lineInput, ' ');

            if (splitString[0] == "maxFitnessLimit") {
                maxFitnessLimit = stoi(splitString[2]);
            } else if (splitString[0] == "popNum") {
                popNum = stoi(splitString[2]);
                numElite = (elitePercentage/100.0) * popNum;
                numMutate = (mutatePercentage/100.0) * popNum;
                numCrossover = (crossoverPercentage/100.0) * popNum;
            } else if (splitString[0] == "elitePercentage") {
                elitePercentage = stoi(splitString[2]);
                numElite = (elitePercentage/100.0) * popNum;
            } else if (splitString[0] == "mutatePercentage") {
                mutatePercentage = stoi(splitString[2]);
                numMutate = (mutatePercentage/100.0) * popNum;
            } else if (splitString[0] == "crossoverPercentage") {
                crossoverPercentage = stoi(splitString[2]);
                numCrossover = (crossoverPercentage/100.0) * popNum;
            }
        }
    } else {
        string error = "Error opening file: " + optionsFilename + "\n" + "ERROR: " + strerror(errno);
        qDebug(error.c_str());
        qDebug("Using default values...");
    }
    optionsFile.close();


    // Setup input file stream
    ifstream readFile;
    readFile.open(filename.c_str());

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
            }

            getline(readFile, lineInput);
            splitString = split(lineInput, ' ');
            if(splitString[0] == "Fitness") {
                testFitness.push_back(stoi(splitString[2]));
            }
        }
    } else {
        string error = "Error opening file: " + filename + "\n" + "ERROR: " + strerror(errno);
        qDebug(error.c_str());
        return 1;
    }
    readFile.close();



    // Does the genetic algorithm for every test case in input file
    for(int case_i=0;case_i<numTestCases;case_i++) {
        int topFitness = 0;

        // Get current sequence and target fitness
        proteinSequence = testSequence[case_i];
        int currSize = proteinSequence.size();
        int targetFitness = testFitness[case_i];
        // If the targetFitness is 0 or higher, it will run infinitely
        if(targetFitness >= 0) {
            targetFitness = INT_MIN;
        }

        // Adjust apocRepeatTimer based on size of input (-10 == x1.0, -14 == x1.4 etc)
        int apocRepeatTriggerAdj = apocRepeatTrigger * (abs(targetFitness) * 0.1);

        // Print out the test case
        string seqOutput1 = "Sequence: " + proteinSequence;
        string seqOutput2 = "Target Fitness: " + to_string(targetFitness);

        qDebug("------- NEW SEQUENCE --------");
        qDebug(seqOutput1.c_str());
        qDebug(seqOutput2.c_str());
        qDebug("-----------------------------");
        qDebug("");

        qDebug("Loading First Generation...");
        qDebug("");

        // Generate initial population
        vector<proteinNode> population;
        population = generateInitialPop(popNum, currSize, maxFitnessLimit);

        // Generate the fitness rating for each member of the population
        for(int i=0;i<popNum;i++) {
            population[i].fitness = getFitnessRating(proteinSequence, population[i].proteinDirection, maxFitnessLimit);
        }

        // Sort the vector based on the fitness rating
        sort(population.begin(), population.end(), ascending());


        // Keeps track of the number of generations, starts at 0 for easy iteration (first generation will be 1)
        int generationNum = 0;

        int currentFitness = 0;
        while(currentFitness > targetFitness) {

            generationNum++;

            // Create second population vector
            vector<proteinNode> nextPopulation;


            // Transfer the elite population
            for(int i=0;i<numElite;i++) {
                nextPopulation.push_back(population[i]);
            }


            // While the population is less than the max size, keep crossing over
            while(nextPopulation.size() < numCrossover) {
                proteinNode parent1 = grabParent(population, numElite);
                proteinNode parent2 = grabParent(population, numElite);
                // Makes sure the second parent isn't the same
                while(parent1.proteinDirection == parent2.proteinDirection) {
                    parent2 = grabParent(population, numElite);
                }

                // If the crossover fails...
                proteinNode child = crossover(parent1, parent2, numToTry, maxFitnessLimit, proteinSequence);
                while(child.proteinDirection == "failed") {
                    // Choose new parents
                    parent1 = grabParent(population, numElite);
                    parent2 = grabParent(population, numElite);
                    // Make sure the second parent isn't the same
                    while(parent1.proteinDirection == parent2.proteinDirection) {
                        parent2 = grabParent(population, numElite);
                    }

                    child = crossover(parent1, parent2, numToTry, maxFitnessLimit, proteinSequence);
                }

                nextPopulation.push_back(child);
            }

            while(nextPopulation.size() < popNum) {
                proteinNode child;
                child.proteinDirection = createRandomSequence(currSize, maxFitnessLimit);

                nextPopulation.push_back(child);
            }


            // Need to sort here in case there is a higher fit after the crossovers
            sort(nextPopulation.begin(), nextPopulation.end(), ascending());


            // Mutates non-elite population randomly
            for(int i=0;i<numMutate;i++) {
                // Grabs index for which non-elite to mutate and mutates it
                //int mutateIndex = (rand() % popNum-numElite) + numElite;
                int mutateIndex = (rand() % popNum);
                string mutated = mutate(nextPopulation[mutateIndex].proteinDirection, numToTry, maxFitnessLimit);

                // While the mutation is not valid, keep choosing a new
                while(mutated == "failed") {
                    mutateIndex = (rand() % popNum);
                    mutated = mutate(nextPopulation[mutateIndex].proteinDirection, numToTry, maxFitnessLimit);
                }

                // Will not save mutation if it is an elite and the fitness is worse, however it will switch if the fitness is equal
                int saveMutation = 1;
                int fitnessMutated = getFitnessRating(proteinSequence, mutated, maxFitnessLimit);
                if(mutateIndex < numElite) {
                    int fitnessOrig = getFitnessRating(proteinSequence, nextPopulation[mutateIndex].proteinDirection, maxFitnessLimit);

                    if(fitnessOrig > fitnessMutated) {
                        saveMutation = 0;
                    }
                }

                if(saveMutation == 1) {
                    nextPopulation[mutateIndex].proteinDirection = mutated;
                    nextPopulation[mutateIndex].fitness = fitnessMutated;
                } else {
                    i--;
                }
            }

            // APOCALYPSE: If apocalypse is 1 and counter is over the repeat trigger limit, kill em all
            if(apocalypse == 1 && apocCounter > apocRepeatTriggerAdj) {
                proteinNode loneSurvivor = nextPopulation[0];

                nextPopulation = generateInitialPop(popNum, currSize, maxFitnessLimit);


                // Generate the fitness rating for each member of the population
                for(int i=0;i<popNum;i++) {
                    nextPopulation[i].fitness = getFitnessRating(proteinSequence, population[i].proteinDirection, maxFitnessLimit);
                }

                // Sort the vector based on the fitness rating
                sort(population.begin(), population.end(), ascending());

                qDebug("---------------------- Oh no an APOCALYPSE!!! -----------------------");
                qDebug("----All but the most fit died. The pop didn't evolve for X cycles----");
                qDebug("------------------------ Time to rebuild... -------------------------");
                qDebug("");

                // The lone survivor evolves on...unless...
                if((rand() % 5) == 0) {
                    nextPopulation[0] = loneSurvivor;
                    qDebug(" !!! There was a lone survivor !!! ");
                    qDebug("");

                    numSurvivors++;
                }

                apocCounter = 0;
                numApoc++;
            } else {
                // Calculate the fitness levels for the nextPopulation
                for(int i=0;i<popNum;i++) {
                    nextPopulation[i].fitness = getFitnessRating(proteinSequence, nextPopulation[i].proteinDirection, maxFitnessLimit);
                }

                // Check for duplicates. Replace with a crossover if it is a duplicate (Ensures duplicate elites don't stack)
                // Done every 50 generations to allow for brief stacking (for higher selection possibility of fit individuals)
                if(generationNum % checkForDupeInterval == 0) {
                    unordered_map<string, int> duplicateCheck;
                    int numDuplicates = 0;
                    for(int i=0;i<popNum;i++) {
                        if(duplicateCheck[nextPopulation[i].proteinDirection] == 1) {
                            proteinNode parent1 = grabParent(nextPopulation, numElite);
                            proteinNode parent2 = grabParent(nextPopulation, numElite);
                            // Makes sure the second parent isn't the same
                            while(parent1.proteinDirection == parent2.proteinDirection) {
                                parent2 = grabParent(nextPopulation, numElite);
                            }

                            // If the crossover fails...
                            proteinNode child = crossover(parent1, parent2, numToTry, maxFitnessLimit, proteinSequence);
                            while(child.proteinDirection == "failed") {
                                // Choose new parents
                                parent1 = grabParent(nextPopulation, numElite);
                                parent2 = grabParent(nextPopulation, numElite);
                                // Make sure the second parent isn't the same
                                while(parent1.proteinDirection == parent2.proteinDirection) {
                                    parent2 = grabParent(nextPopulation, numElite);
                                }

                                child = crossover(parent1, parent2, numToTry, maxFitnessLimit, proteinSequence);
                            }

                            nextPopulation[i] = child;
                            numDuplicates++;
                        } else {
                            duplicateCheck[nextPopulation[i].proteinDirection] = 1;
                        }
                    }

                    string duplicateText = " !!! Number of Duplicates Removed: " + to_string(numDuplicates) + " !!! ";
                    qDebug(duplicateText.c_str());
                    qDebug("");
                }

                // Sort the vector based on the fitness rating
                sort(nextPopulation.begin(), nextPopulation.end(), ascending());

                if(apocLastFitness == nextPopulation[0].fitness) {
                    apocCounter++;
                } else {
                    apocCounter = 0;
                    apocLastFitness = nextPopulation[0].fitness;
                }

                currentFitness = nextPopulation[0].fitness;
                population = nextPopulation;

                if(currentFitness < topFitness) {
                    topFitness = currentFitness;
                }

                // Display stats in console
                string generation = "-------------- Generation: " + to_string(generationNum) + " --------------";
                string currentFitString = "Fitness:    " + to_string(currentFitness) + " / " + to_string(targetFitness) + "   TopFit: " + to_string(topFitness);
                string currentDirections = "Directions: " + population[0].proteinDirection;
                string currentSequence = "Sequence:   " + proteinSequence;
                string currentFinished = "------ (Done: " + to_string(numCompleted) + "  Apoc: " + to_string(numApoc) + "  Survivors: " + to_string(numSurvivors) + ") ------";

                qDebug(generation.c_str());
                qDebug(currentFitString.c_str());
                qDebug(currentDirections.c_str());
                qDebug(currentSequence.c_str());
                qDebug(currentFinished.c_str());

                qDebug("");
            }





            // Display image of best fit in generation

            QPicture pi;

            // Create scalable image of projected protein
            // If drawRand == 1, draw a protein from the population
            int fitness;
            if(drawRand == 1) {
                int randIndex = rand() % (int)(popNum * (drawPercentage/100.0));
                pi = drawProtein(proteinSequence, population[randIndex].proteinDirection, maxFitnessLimit, pixelSpacing);
                fitness = population[randIndex].fitness;
            } else {
                pi = drawProtein(proteinSequence, population[0].proteinDirection, maxFitnessLimit, pixelSpacing);
                fitness = population[0].fitness;
            }


            // Create the fitness sublabel
            string fitText = "Fitness: " + to_string(fitness);

            // Setup child label for displaying the fitness level
            QLabel r(&l);
            QFont f("Arial", 16, QFont::Bold);
            r.setMargin(10);
            r.setFont(f);
            r.setAlignment(Qt::AlignTop);
            r.setText(QString::fromStdString(fitText));
            r.show();

            // Draw parent QLabel, containing the image and fitness sub-QLabel
            l.setPicture(pi);
            l.show();

            // Update window by displaying new drawing
            a.processEvents();
        }
        numSurvivors = 0;
        topFitness = 0;
        numApoc = 0;
        numCompleted++;


        qDebug("");
        qDebug("");
        qDebug("");
        qDebug("");
        qDebug("");
        qDebug("");
        qDebug("---------------------- Pinnacle Evolution: Achieved -----------------------");
        qDebug("-------------- We have surpassed all that can be surpassed.  --------------");
        qDebug("----------------- Time to create new life from scratch... -----------------");
        qDebug("");
        qDebug("");
        qDebug("");
        qDebug("");
        qDebug("");
        qDebug("");

    }

    return 1;
}


// Tries numToTry times to mutate a random index of the proteinDirection string
string mutate(string proteinDirection, int numToTry, int maxFitnessLimit) {

    for(int i=0;i<numToTry;i++) {
        // Reset string, generate random index
        string testProteinDir = proteinDirection;
        int randomIndex = rand() % (proteinDirection.size() - 1);
        char charAtIndex = testProteinDir[randomIndex];

        char randomDir = '0' + ((rand() % 4) + 1);

        // Make sure the direction is not the same
        while(testProteinDir[randomIndex] == randomDir) {
            randomDir = '0' + ((rand() % 4) + 1);
        }

        // Calculate offset between original and new directions
        int offset = abs((int)randomDir - (int)testProteinDir[randomIndex]);
        // Generate twistDirection for mutation twist, and sweep direction for direction of sweep
        // For both, 0 is left and 1 is right
        int twistDirection = rand() % 2;
        int sweepDirection = rand() % 2;

        // Do loop to perform mutation of each index after or before the
        if(sweepDirection == 0) {
            // Go to the left
            for(int i=randomIndex;i>=0;i--) {
                if(twistDirection == 0) {
                    // Calculate new dir and make sure it is valid
                    int newDir = (int)testProteinDir[i] - offset;
                    if(newDir < 1) {
                        newDir += 4;
                    }
                    testProteinDir[i] = '0' + newDir;
                } else {
                    int newDir = (int)testProteinDir[i] + offset;
                    if(newDir > 4) {
                        newDir -= 4;
                    }
                    testProteinDir[i] = '0' + newDir;
                }
            }
        } else {
            // Go to the right
            // Note, does not check last index (should remain 0)
            for(int i=randomIndex;i<proteinDirection.size()-1;i++) {
                if(!testProteinDir[i] == '0') {
                    if(twistDirection == 0) {
                        // Calculate new dir and make sure it is valid
                        int newDir = (int)testProteinDir[i] - offset;
                        if(newDir < 1) {
                            newDir += 4;
                        }
                        testProteinDir[i] = '0' + newDir;
                    } else {
                        int newDir = (int)testProteinDir[i] + offset;
                        if(newDir > 4) {
                            newDir -= 4;
                        }
                        testProteinDir[i] = '0' + newDir;
                    }
                }
            }
        }

        // If the mutation is successful, return it
        if(!collisionDetection(testProteinDir, maxFitnessLimit)) {
            return testProteinDir;
        }
    }
    // If the numToTry runs out, return failed
    return "failed";
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
proteinNode grabParent(vector<proteinNode> population, int numElite) {
    int divBy = 8;
    int chance = (rand() % divBy) + 1;

    int toGrab = -1;
    // 25% Chance
    int tempChance = numElite * divBy;

    int randomChance = rand() % tempChance;
    if(randomChance < tempChance/chance) {
        toGrab = randomChance;
    } else {
        toGrab = tempChance/chance + rand() % (population.size() - tempChance/2);
    }

    return population[toGrab];
}


// Crosses 2 proteins over, if they can be crossed. Otherwise, returns "failed" in the proteinDirection
proteinNode crossover(proteinNode parent1, proteinNode parent2, int numToTry, int maxFitnessLimit, string proteinSequence) {
    int sizeParents = parent1.proteinDirection.size();

    proteinNode parent1Mod;
    proteinNode parent2Mod;

    proteinNode child;

    // Loops for number of attempts allowed with these 2 proteinNodes
    for(int i=0;i<numToTry;i++) {
        parent1Mod = parent1;
        parent2Mod = parent2;

        // Index to start and end, direction to go in string, direction to twist when rotate checking
        int randIndexL = rand() % sizeParents;
        int randIndexH = rand() % sizeParents;
        while(randIndexL == randIndexH) {
            randIndexL = rand() % sizeParents;
            randIndexH = rand() % sizeParents;
        }

        if(randIndexL > randIndexH) {
            int temp = randIndexL;
            randIndexL = randIndexH;
            randIndexH = temp;
        }


        int sweepDirection = rand() % 2;
        int twistDirection = rand() % 2;

        if(sweepDirection == 0) {
            for(int j=0;j<4;j++) {
                for(int k=randIndexH;k>=randIndexL;k--) {
                    // Modify the value based on the random twistDirection chosen
                    int tempDirection;
                    if(twistDirection == 1) {
                        tempDirection = (int)(parent2Mod.proteinDirection[k]) + j - 48;
                    } else {
                        tempDirection = (int)(parent2Mod.proteinDirection[k]) - j - 48;
                    }

                    // Account for numbers outside the range
                    if (tempDirection > 4) {
                        tempDirection -= 4;
                    } else if (tempDirection < 1) {
                        tempDirection += 4;
                    }

                    parent1Mod.proteinDirection[k] = '0' + tempDirection;
                }

                if(!collisionDetection(parent1Mod.proteinDirection, maxFitnessLimit)) {
                    return parent1Mod;
                }

                parent1Mod = parent1;
                parent2Mod = parent2;
            }
        } else {
            for(int j=0;j<4;j++) {
                for(int k=randIndexL;k<randIndexH;k++) {
                    int tempDirection;
                    if(twistDirection == 1) {
                        tempDirection = (int)(parent2Mod.proteinDirection[k]) + j - 48;
                    } else {
                        tempDirection = (int)(parent2Mod.proteinDirection[k]) - j - 48;
                    }

                    if (tempDirection > 4) {
                        tempDirection -= 4;
                    } else if (tempDirection < 1) {
                        tempDirection += 4;
                    }

                    parent1Mod.proteinDirection[k] = '0' + tempDirection;
                }

                if(!collisionDetection(parent1Mod.proteinDirection, maxFitnessLimit)) {
                    parent1Mod.fitness = getFitnessRating(proteinSequence, parent1Mod.proteinDirection, maxFitnessLimit);
                    return parent1Mod;
                }

                parent1Mod = parent1;
                parent2Mod = parent2;
            }
        }
    }

    child.proteinDirection = "failed";
    child.fitness = 0;
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

    for(int i=0;i<proteinSequence.size() - 1;i++) {
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


    // Do iteration of dotted line and dot drawing for the last point
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
