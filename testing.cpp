#include "testing.h"

testing::testing()
{
    // proteinSequence = h=Hydrophobic(special/red) p=Hydrophilic(black)
    string proteinSequence  = "pphpphhpphhppppphhhhhhhhhhpppppphhpphhpphpphhhhh";

    // Directions the protein travels from each node
    // 1=North 2=East 3=South 4=West, 0=Nowhere(end)
    string proteinDirection = "232332221441123212323322212232111112333221141120";

    const int maxFitnessLimit = 1024;

    int pixelSpacing = 25;


    // Display best fit in generation
    QPicture pi = drawProtein(proteinSequence, proteinDirection, maxFitnessLimit, pixelSpacing);
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
}


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

    for(int i=0;i<proteinSequence.size();i++) {
        char currentType = proteinSequence[i];
        char currentDirection = proteinDirection[i];


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



        // Finally draw the dot for the type of amino acid on top of the line
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

    p.end();

    return pi;
}
