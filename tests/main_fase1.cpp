#include "MetadataSensor.h"

#include <iostream>

int main () {
    MetadataSensor meta1("A2", "microVolts", 60);
    
    meta1.setCodeOfChannel("C2");
    
    std::cout << meta1.ConvertIndexSecs(2.23) << std::endl;

    std::cout << "estas corriendo el progarama De pruebas" << std::endl; 


    return 0;
};
