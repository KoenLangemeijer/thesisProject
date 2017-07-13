//
// Created by Koen Langemeijer on 13/07/2017.
//

#ifndef TUDATBUNDLE_CREATEINITIALCONDITIONS_H
#define TUDATBUNDLE_CREATEINITIALCONDITIONS_H



#include "createInitialConditions.cpp"


void createInitialConditions( int lagrangePointNr, string orbitType, double amplitudeOne, double amplitudeTwo,
                              const double primaryGravitationalParameter, const double secondaryGravitationalParameter,
                              double maxPositionDeviationFromPeriodicOrbit, double maxVelocityDeviationFromPeriodicOrbit,
                              double maxDeviationEigenvalue);



#endif //TUDATBUNDLE_CREATEINITIALCONDITIONS_H