#include <iostream>
#include "ChannelSignal.h"
#include "SlicingWindow.h"

int main() {
    const double demoSamples[6] = {1.0, 2.5, 4.0, 3.5, 2.0, 1.0};
    ChannelSignal channel("CH1", "uV", 100.0, demoSamples, 6);
    const ChannelStadistics& stats = channel.getProcessedStatistics();

    std::cout << "Motor de Series de Tiempo - Fase 1 operativa\n";
    std::cout << "Canal: " << channel.getMetadata().getCodeOfChannel()
              << " | Muestras: " << channel.getNumberOfSamples()
              << " | Media: " << stats.getAverage()
              << " | Desv. Est.: " << stats.getStandardDesviation() << "\n";

    return 0;
}
