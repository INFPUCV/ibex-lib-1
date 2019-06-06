/*      Crowding Distance Implementation
 *                  v1.0.1
 *  
 *              Author: Nyuku
 */

#include <iostream>
#include "ibex_CrowdingDistance.h"

using namespace std;

namespace ibex{


    void NyuCrowdingDistance::getCrowdingDistance(
        std::multiset<Cell*, max_distanceBeam>& nextBuffer,
        std::priority_queue<Cell*, std::vector<Cell*>, max_distanceBeam >& currentBuffer, 
        std::priority_queue<Cell*, std::vector<Cell*>, max_distanceBeam >& globalBuffer,
        //int currentBufferSize,
        int returnSize
        ){

        //First, we take out the dominated ones.
        //removeDominated(nextBuffer, currentBuffer); //We do this now in the BeamSearchBufferMOP Class.

        std::multiset<CDBox*, sortByCrowdingDistance> cdSet;
        for(std::multiset<Cell*, crowding_distanceBeam>::iterator it = nextBuffer.begin();
        it != nextBuffer.end(); it++)
        {

            CDBox* cdBox= (CDBox*)malloc(sizeof(CDBox));
            cdBox->C = *it;

            //If they are the first/last one, set their Crowding Distances to Infinite
            if(*it == *nextBuffer.begin() || *next(it) == *nextBuffer.end()){
                std::cout << "SAME" << endl;
                cdBox->crowding_distance = std::numeric_limits<double>::infinity();
            }
            else{ //If not, then calculate them.
                 int n = (*it)->box.size();
                Cell* first = *nextBuffer.begin();
                Cell* last = *prev(nextBuffer.end());


                /* Here we calculate the crowding distance. We won't use a for, since we already know that there's only 2 objectives (y1, y2)*/
                //Primer Objetivo
                cdBox->crowding_distance += ((*std::prev(it))->box[n-1].lb() - (*std::next(it))->box[n-1].lb())/((first->box[n-1].lb() - last->box[n-1].lb()));
                //Segundo objetivo
                cdBox->crowding_distance += ((*std::prev(it))->box[n-2].lb() - (*std::next(it))->box[n-2].lb())/((first->box[n-2].lb() - last->box[n-2].lb()));
            }

            //Inserts the CDBox node into the set
            cdSet.insert(cdBox);
        }
        
        //Returns a set with a size of -currentBuffer- Cells. The first -currentBuffer- cells in cdBuffer get returned (First and last + currentBuffer-2 other ones)
        int i=0;

        //Here, we are going to insert the 8 cells with the highest crowding distance, and insert them into currentBuffer.
        for(auto cdbox: cdSet){
            currentBuffer.push(cdbox->C);
            nextBuffer.erase(cdbox->C); //Elimina la cell del nextBuffer, ya que solo interesa que este en el return!
            std::cout << i << ", " << cdbox->crowding_distance << ", "
            		<< cdbox->C->box[cdbox->C->box.size()-1].lb() << "," << cdbox->C->box[cdbox->C->box.size()-2].lb()<< endl;
        	i++;
        	if(i==returnSize) break;
        }

        //Since the top X are removed from the nextBuffer, we move nextBuffer into globalBuffer. (leftovers)
        if(!nextBuffer.empty()){
            for(auto cell : nextBuffer){
                globalBuffer.push(cell);
                nextBuffer.erase(cell);
            }
        }
        //getchar();
    }

    bool NyuCrowdingDistance::isDominated(Cell* a, Cell* b){
        int n = a->box.size();
        return (a->box[n-1].lb() <= b->box[n-1].lb() && a->box[n-2].lb() <= b->box[n-2].lb());
    }

    //Removes the dominated cells from nextBuffers and adds them into globalBuffer.
    void NyuCrowdingDistance::removeDominated(
    std::multiset<Cell*, max_distanceBeam>& nextBuffer,
    std::priority_queue<Cell*, std::vector<Cell*>, max_distanceBeam >& globalBuffer
    ){
        //std::multiset<Cell*, max_distanceBeam> nextBufferCopy;
        //Here we take the dominateds out of nextBuffer and insert them into globalBuffer.
        for(auto a : nextBuffer){
            for(auto b : nextBuffer){
                if(a != b && isDominated(a, b)){
                    globalBuffer.push(a);
                    nextBuffer.erase(a);
                }
            }
        }
    }


}
