#include "MetadataSensor.h"
#include "ChannelStadistics.h"

#include <iostream>
#include <ostream>

int main () {
    MetadataSensor meta1("A2", "microVolts", 60);
    
    meta1.setCodeOfChannel("C2");
    
    std::cout << meta1.ConvertIndexSecs(2.23) << std::endl;

    std::cout << "estas corriendo el progarama De pruebas" << std::endl; 

    double* data = new double[10] {1, 2, 3, 5, 1, 6, 1, 2, 3, 1};

    ChannelStadistics stat(data, 10);

    std::cout << stat.getAverage() << " " << stat.getStandardDesviation() << " "  << stat.getMaximum() << " " << stat.getTotalEnergy() << " " << std::endl;

    return 0;
};
