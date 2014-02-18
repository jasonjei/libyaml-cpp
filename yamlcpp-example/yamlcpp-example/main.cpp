//
//  main.cpp
//  yamlcpp-example
//
//  Created by Jason Hung on 2/17/14.
//  Copyright (c) 2014 Jason Hung. All rights reserved.
//

#include <iostream>
#include "MapObject.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, const char * argv[])
{
    // - name: Ogre
    //   position: [0, 5, 0]
    //   powers:
    //     - name: Club
    //       damage: 10
    //     - name: Fist
    //       damage: 8
    // - name: Dragon
    //   position: [1, 0, 10]
    //   powers:
    //     - name: Fire Breath
    //       damage: 25
    //     - name: Claws
    //       damage: 15
    // - name: Wizard
    //   position: [5, -3, 0]
    //   powers:
    //     - name: Acid Rain
    //       damage: 50
    //     - name: Staff
    //       damage: 3

    FILE *fh;
    fh = fopen("monsters.yml", "rb");
    
    MapObject mapObject = MapObject::processYaml(fh);
    
    if (mapObject._type == MapObject::MAP_OBJ_FAILED) {
        std::cout << "Failed opening test file!" << std::endl;
        return 0;
    }
    
    if (mapObject._type == MapObject::MAP_OBJ_VECTOR && mapObject.mapObjects.size() > 0
            && mapObject.mapObjects.front()._type == MapObject::MAP_OBJ_MAP
            && mapObject.mapObjects.front().mapPtr->map.count("name")
            && mapObject.mapObjects.front().mapPtr->map.count("position")) {
        
        std::cout << "The first monster is " << mapObject.mapObjects[0].mapPtr->map["name"].value << "\n";
        std::cout << "It has the following positions: " << std::endl;
        
        for (int i=0; i < mapObject.mapObjects[0].mapPtr->map["position"].mapObjects.size(); i++) {
            std::cout << mapObject.mapObjects[0].mapPtr->map["position"].mapObjects.at(i).value;
            
            if (i+1 == mapObject.mapObjects[0].mapPtr->map["position"].mapObjects.size())
                std::cout << std::endl;
            else
                std::cout << ", ";
        }
        
    }
    return 0;
}

