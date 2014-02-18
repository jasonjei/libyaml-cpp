### libyaml-cpp
*** A lightweight C++ wrapper for libyaml

---
Motivation for the project

We wanted to use yaml for our code instead of using the Windows registry. However, the choices for C++ yaml parsing and emitting were limited. One prominent choice would require us to use boost.  However, using libyaml by itself using tokens and lexical parsing also seemed tedious. We wanted a Ruby-esque way to read yaml files.
---

The difficulty of implementing a C++ yaml reader that is like one in Ruby is that we need a single container that can have a vector type, string type, or map type.  Thus, we have a class called MapObject which implements all three types. 

It is up to the user to test that the object type using _type to determine whether the current node is a map, vector, or string.  

For example, you might use it like so to read a YAML file

- name: Ogre
  position: [0, 5, 0]
  powers:
    - name: Club
      damage: 10
    - name: Fist
      damage: 8
- name: Dragon
  position: [1, 0, 10]
  powers:
    - name: Fire Breath
      damage: 25
    - name: Claws
      damage: 15
- name: Wizard
  position: [5, -3, 0]
  powers:
    - name: Acid Rain
      damage: 50
    - name: Staff
      damage: 3 

```
FILE *fh;
fh = fopen("monsters.yml", "rb");
    
MapObject mapObject = MapObject::processYaml(fh);

std::cout << "The first monster is " << mapObject.mapObjects[0].mapPtr->map["name"].value << "\n";
```

Sorry for the lack of documentation. That's where you guys come in :)