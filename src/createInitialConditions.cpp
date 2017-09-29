#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <Eigen/QR>
#include <Eigen/Dense>

#include "applyDifferentialCorrection.h"
#include "checkEigenvalues.h"
#include "computeEigenvalues.h"
#include "propagateOrbit.h"
#include "richardsonThirdOrderApproximation.h"
#include "writePeriodicOrbitToFile.h"

void appendResultsVector(
        const double jacobiEnergy, const double orbitalPeriod, const Eigen::VectorXd initialStateVector,
        const Eigen::VectorXd stateVectorInclSTM, std::vector< Eigen::VectorXd >& initialConditions )
{
    Eigen::VectorXd tempInitialCondition = Eigen::VectorXd( 44 );

    // Add Jacobi energy and orbital period
    tempInitialCondition( 0 ) = jacobiEnergy;
    tempInitialCondition( 1 ) = orbitalPeriod;

    // Add initial condition of periodic solution
    for (int i = 0; i <= 5; i++){
        tempInitialCondition( i + 2 ) = initialStateVector( i );
    }

    // Add Monodromy matrix
    for (int i = 6; i <= 41; i++){
        tempInitialCondition( i + 2 ) = stateVectorInclSTM(i);
    }

    initialConditions.push_back(tempInitialCondition);
}

void appendDifferentialCorrectionResultsVector(
        const double jacobiEnergyHalfPeriod,  const Eigen::VectorXd differentialCorrectionResult,
        std::vector< Eigen::VectorXd >& differentialCorrections )
{

    Eigen::VectorXd tempDifferentialCorrection = Eigen::VectorXd( 9 );
    tempDifferentialCorrection( 0 ) = differentialCorrectionResult(14);  // numberOfIterations
    tempDifferentialCorrection( 1 ) = jacobiEnergyHalfPeriod;  // jacobiEnergyHalfPeriod
    tempDifferentialCorrection( 2 ) = differentialCorrectionResult(13);  // currentTime
    for (int i = 7; i <= 12; i++){
        tempDifferentialCorrection( i - 4 ) = differentialCorrectionResult( i );  // halfPeriodStateVector
    }

    differentialCorrections.push_back(tempDifferentialCorrection);

}

Eigen::Vector7d getInitialStateVectorGuess( int librationPointNr, std::string orbitType, const int guessIteration )
{
    Eigen::Vector7d richardsonThirdOrderApproximationResult;
    if( guessIteration == 0 )
    {
        if (orbitType == "horizontal") {
            if (librationPointNr == 1) {
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("horizontal", 1, 1.0e-3);
            } else if (librationPointNr == 2) {
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("horizontal", 2, 1.0e-4);
            }
        } else if (orbitType == "vertical"){
            if (librationPointNr == 1){
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("vertical", 1, 1.0e-1);
            } else if (librationPointNr == 2){
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("vertical", 2, 1.0e-1);
            }
        } else if (orbitType == "halo") {
            if (librationPointNr == 1) {
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("halo", 1, 1.1e-1, 3.0);
            } else if (librationPointNr == 2) {
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("halo", 2, 1.5e-1);
            }
        }
    }
    else if( guessIteration == 1 )
    {

        if (orbitType == "horizontal") {
            if (librationPointNr == 1) {
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("horizontal", 1, 1.0e-4);
            } else if (librationPointNr == 2) {
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("horizontal", 2, 1.0e-3);
            }
        } else if (orbitType == "vertical"){
            if (librationPointNr == 1){
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("vertical", 1, 2.0e-1);
            } else if (librationPointNr == 2){
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("vertical", 2, 2.0e-1);
            }
        } else if (orbitType == "halo") {
            if (librationPointNr == 1) {
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("halo", 1, 1.2e-1, 3.0);
            } else if (librationPointNr == 2) {
                richardsonThirdOrderApproximationResult = richardsonThirdOrderApproximation("halo", 2, 1.6e-1);
            }
        }
    }

    return richardsonThirdOrderApproximationResult;
}

void createInitialConditions( int librationPointNr, std::string orbitType,
                              const double primaryGravitationalParameter = tudat::celestial_body_constants::EARTH_GRAVITATIONAL_PARAMETER,
                              const double secondaryGravitationalParameter = tudat::celestial_body_constants::MOON_GRAVITATIONAL_PARAMETER,
                              double maxPositionDeviationFromPeriodicOrbit = 1.0e-12, double maxVelocityDeviationFromPeriodicOrbit = 1.0e-12,
                              double maxEigenvalueDeviation = 1.0e-3 )
{
    std::cout << "\nCreate initial conditions:\n" << std::endl;

    // Set output maximum precision
    std::cout.precision(std::numeric_limits<double>::digits10);

    // Initialize state vectors and orbital periods
    double orbitalPeriod               = 0.0;
    double jacobiEnergy                = 0.0;
    double jacobiEnergyHalfPeriod      = 0.0;
    double pseudoArcLengthCorrection   = 0.0;
    bool continueNumericalContinuation = true;
    Eigen::VectorXd initialStateVector = Eigen::VectorXd::Zero(6);
    Eigen::VectorXd delta              = Eigen::VectorXd::Zero(7);
    Eigen::VectorXd stateVectorInclSTM = Eigen::VectorXd::Zero(42);
    std::vector< Eigen::VectorXd > initialConditions;
    std::vector< Eigen::VectorXd > differentialCorrections;
    Eigen::VectorXd                     differentialCorrectionResult;

    // Define massParameter
    massParameter = tudat::gravitation::circular_restricted_three_body_problem::computeMassParameter(
                primaryGravitationalParameter, secondaryGravitationalParameter );

    // Split input parameters
    Eigen::Vector7d richardsonThirdOrderApproximationResult =
            getInitialStateVectorGuess( librationPointNr, orbitType, 0 );
    initialStateVector = richardsonThirdOrderApproximationResult.segment(0,6);
    orbitalPeriod      = richardsonThirdOrderApproximationResult(6);

    // Correct state vector guesses
    differentialCorrectionResult = applyDifferentialCorrection(
                librationPointNr, orbitType, initialStateVector, orbitalPeriod, massParameter,
                maxPositionDeviationFromPeriodicOrbit, maxVelocityDeviationFromPeriodicOrbit );
    initialStateVector           = differentialCorrectionResult.segment(0,6);
    orbitalPeriod                = differentialCorrectionResult(6);

    // Save number of iterations, jacobi energy, time of integration and the half period state vector
    {

        jacobiEnergyHalfPeriod       = tudat::gravitation::computeJacobiEnergy(massParameter, differentialCorrectionResult.segment( 7, 6 ) );
        appendDifferentialCorrectionResultsVector( jacobiEnergyHalfPeriod, differentialCorrectionResult, differentialCorrections );

        // Propagate the initialStateVector for a full period and write output to file.
        stateVectorInclSTM = writePeriodicOrbitToFile( initialStateVector, librationPointNr, orbitType, 0, orbitalPeriod, massParameter);

        // Save jacobi energy, orbital period, initial condition, and eigenvalues
        jacobiEnergy = tudat::gravitation::computeJacobiEnergy(massParameter, stateVectorInclSTM.segment(0,6));
        appendResultsVector( jacobiEnergy, orbitalPeriod, initialStateVector, stateVectorInclSTM, initialConditions );
    }

    // Split input parameters
    richardsonThirdOrderApproximationResult =
            getInitialStateVectorGuess( librationPointNr, orbitType, 1 );
    initialStateVector = richardsonThirdOrderApproximationResult.segment(0,6);
    orbitalPeriod = richardsonThirdOrderApproximationResult(6);

    // Correct state vector guesses
    differentialCorrectionResult = applyDifferentialCorrection(
                librationPointNr, orbitType, initialStateVector, orbitalPeriod, massParameter,
                maxPositionDeviationFromPeriodicOrbit, maxVelocityDeviationFromPeriodicOrbit);
    initialStateVector           = differentialCorrectionResult.segment(0,6);
    orbitalPeriod                = differentialCorrectionResult(6);

    {
        // Save number of iterations, jacobi energy, time of integration and the half period state vector
        jacobiEnergyHalfPeriod       = tudat::gravitation::computeJacobiEnergy(massParameter, differentialCorrectionResult.segment(7,6));

        appendDifferentialCorrectionResultsVector( jacobiEnergyHalfPeriod, differentialCorrectionResult, differentialCorrections );


        // Propagate the initialStateVector for a full period and write output to file.
        stateVectorInclSTM = writePeriodicOrbitToFile( initialStateVector, librationPointNr, orbitType, 1, orbitalPeriod, massParameter);
        jacobiEnergy = tudat::gravitation::computeJacobiEnergy(massParameter, stateVectorInclSTM.segment(0,6));

        // Save jacobi energy, orbital period, initial condition, and eigenvalues
        appendResultsVector( jacobiEnergy, orbitalPeriod, initialStateVector, stateVectorInclSTM, initialConditions );
    }
    // Set exit parameters of continuation procedure
    int numberOfInitialConditions = 2;
    int maximumNumberOfInitialConditions = 4000;

    while (numberOfInitialConditions < maximumNumberOfInitialConditions and continueNumericalContinuation)
    {

        continueNumericalContinuation = false;

        delta = Eigen::VectorXd::Zero(7);
        delta.segment( 0, 6 ) = initialConditions[initialConditions.size()-1].segment( 2, 6 ) -
                initialConditions[initialConditions.size()-2].segment( 2, 6 );
        delta( 6 ) = initialConditions[initialConditions.size()-1]( 1 ) -
                initialConditions[initialConditions.size()-2]( 1 );

        pseudoArcLengthCorrection = 1e-4 / sqrt(pow(delta(0),2) + pow(delta(1),2) + pow(delta(2),2));

        std::cout << "pseudoArcCorrection: " << pseudoArcLengthCorrection << std::endl;

        // Apply numerical continuation
        initialStateVector = initialConditions[initialConditions.size()-1].segment( 2, 6 ) +
                delta.segment( 0, 6 ) * pseudoArcLengthCorrection;
        orbitalPeriod = initialConditions[initialConditions.size()-1]( 1 ) +
                delta(6) * pseudoArcLengthCorrection;

        // Correct state vector guesses
        differentialCorrectionResult = applyDifferentialCorrection(
                    librationPointNr, orbitType, initialStateVector, orbitalPeriod, massParameter,
                    maxPositionDeviationFromPeriodicOrbit, maxVelocityDeviationFromPeriodicOrbit);
        if (differentialCorrectionResult == Eigen::VectorXd::Zero( 15 ) )
        {
            continueNumericalContinuation = false;
            std::cout << "\n\nNUMERICAL CONTINUATION STOPPED DUE TO EXCEEDING MAXIMUM NUMBER OF ITERATIONS\n\n" << std::endl;
            break;
        }
        initialStateVector           = differentialCorrectionResult.segment(0,6);
        orbitalPeriod                = differentialCorrectionResult(6);

        // Save number of iterations, jacobi energy, time of integration and the half period state vector
        jacobiEnergyHalfPeriod       = tudat::gravitation::computeJacobiEnergy(massParameter, differentialCorrectionResult.segment(7,6));

        appendDifferentialCorrectionResultsVector( jacobiEnergyHalfPeriod, differentialCorrectionResult, differentialCorrections );

        // Propagate the initialStateVector for a full period and write output to file.
        stateVectorInclSTM = writePeriodicOrbitToFile( initialStateVector, librationPointNr, orbitType, numberOfInitialConditions, orbitalPeriod, massParameter);

        jacobiEnergy = tudat::gravitation::computeJacobiEnergy(massParameter, initialStateVector);
        appendResultsVector( jacobiEnergy, orbitalPeriod, initialStateVector, stateVectorInclSTM, initialConditions );

        // Check eigenvalue condition (at least one pair equalling a real one)
        // Exception for the horizontal Lyapunov family in Earth-Moon L2: eigenvalue may be of module one instead of a real one to compute a more extensive family
        if ( librationPointNr == 2 and orbitType == "horizontal" )
        {
            continueNumericalContinuation = checkEigenvalues(stateVectorInclSTM, maxEigenvalueDeviation, true);
        }
        else
        {
            continueNumericalContinuation = checkEigenvalues(stateVectorInclSTM, maxEigenvalueDeviation, false);
        }
        numberOfInitialConditions += 1;
    }

    // Prepare file for initial conditions
    remove(("../data/raw/orbits/L" + std::to_string(librationPointNr) + "_" + orbitType + "_initial_conditions.txt").c_str());
    std::ofstream textFileInitialConditions;
    textFileInitialConditions.open(("../data/raw/orbits/L" + std::to_string(librationPointNr) + "_" + orbitType + "_initial_conditions.txt"));
    textFileInitialConditions.precision(std::numeric_limits<double>::digits10);

    // Prepare file for differential correction
    remove(("../data/raw/orbits/L" + std::to_string(librationPointNr) + "_" + orbitType + "_differential_correction.txt").c_str());
    std::ofstream textFileDifferentialCorrection;
    textFileDifferentialCorrection.open(("../data/raw/orbits/L" + std::to_string(librationPointNr) + "_" + orbitType + "_differential_correction.txt"));
    textFileDifferentialCorrection.precision(std::numeric_limits<double>::digits10);

    // Write initial conditions to file
    for (unsigned int i=0; i<initialConditions.size(); i++) {

        textFileInitialConditions << std::left << std::scientific                                          << std::setw(25)
                                  << initialConditions[i][0]  << std::setw(25) << initialConditions[i][1]  << std::setw(25)
                                  << initialConditions[i][2]  << std::setw(25) << initialConditions[i][3]  << std::setw(25)
                                  << initialConditions[i][4]  << std::setw(25) << initialConditions[i][5]  << std::setw(25)
                                  << initialConditions[i][6]  << std::setw(25) << initialConditions[i][7]  << std::setw(25)
                                  << initialConditions[i][8]  << std::setw(25) << initialConditions[i][9]  << std::setw(25)
                                  << initialConditions[i][10] << std::setw(25) << initialConditions[i][11] << std::setw(25)
                                  << initialConditions[i][12] << std::setw(25) << initialConditions[i][13] << std::setw(25)
                                  << initialConditions[i][14] << std::setw(25) << initialConditions[i][15] << std::setw(25)
                                  << initialConditions[i][16] << std::setw(25) << initialConditions[i][17] << std::setw(25)
                                  << initialConditions[i][18] << std::setw(25) << initialConditions[i][19] << std::setw(25)
                                  << initialConditions[i][20] << std::setw(25) << initialConditions[i][21] << std::setw(25)
                                  << initialConditions[i][22] << std::setw(25) << initialConditions[i][23] << std::setw(25)
                                  << initialConditions[i][24] << std::setw(25) << initialConditions[i][25] << std::setw(25)
                                  << initialConditions[i][26] << std::setw(25) << initialConditions[i][27] << std::setw(25)
                                  << initialConditions[i][28] << std::setw(25) << initialConditions[i][29] << std::setw(25)
                                  << initialConditions[i][30] << std::setw(25) << initialConditions[i][31] << std::setw(25)
                                  << initialConditions[i][32] << std::setw(25) << initialConditions[i][33] << std::setw(25)
                                  << initialConditions[i][34] << std::setw(25) << initialConditions[i][35] << std::setw(25)
                                  << initialConditions[i][36] << std::setw(25) << initialConditions[i][37] << std::setw(25)
                                  << initialConditions[i][38] << std::setw(25) << initialConditions[i][39] << std::setw(25)
                                  << initialConditions[i][40] << std::setw(25) << initialConditions[i][41] << std::setw(25)
                                  << initialConditions[i][42] << std::setw(25) << initialConditions[i][43] << std::endl;

        textFileDifferentialCorrection << std::left << std::scientific   << std::setw(25)
                                       << differentialCorrections[i][0]  << std::setw(25) << differentialCorrections[i][1]  << std::setw(25)
                                       << differentialCorrections[i][2]  << std::setw(25) << differentialCorrections[i][3]  << std::setw(25)
                                       << differentialCorrections[i][4]  << std::setw(25) << differentialCorrections[i][5]  << std::setw(25)
                                       << differentialCorrections[i][6]  << std::setw(25) << differentialCorrections[i][7]  << std::setw(25)
                                       << differentialCorrections[i][8]  << std::setw(25) << std::endl;
    }
}
