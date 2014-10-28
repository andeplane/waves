#include "wavesolver.h"
#include <cmath>

float WaveSolver::averageValue() const
{
    return m_averageValue;
}

void WaveSolver::calculateWalls()
{
    for(unsigned int i=0;i<gridSize();i++) {
        for(unsigned int j=0;j<gridSize();j++) {
            int oldValue = m_walls[i][j];
            m_walls[i][j] = m_ground[i][j] > m_averageValue;
            if(oldValue != m_walls[i][j]) {
                m_solutionPrevious[i][j] = m_solution[i][j] = m_averageValue;
            }
        }
    }
}

void WaveSolver::calculateMean()
{
    m_averageValue = 0;
    unsigned int count = 0;
    for(unsigned int i=0;i<gridSize();i++) {
        for(unsigned int j=0;j<gridSize();j++) {
            if(!m_walls[i][j]) {
                m_averageValue += m_solutionNext[i][j];
                count++;
            }
        }
    }

    m_averageValue /= count;
}

WaveSolver::WaveSolver() :
    m_dampingFactor(1),
    m_gridSize(0),
    m_dr(0),
    m_length(2)
{
    setGridSize(100);
    float amplitude = 0.1;
    float rMin = -1.0;
    float rMax = -1.0;
    float length = rMax-rMin;
    setLength(length);
    float x0 = 0;
    float y0 = 0;
    float standardDeviation = 0.05;
    double maxValue = 0;
    applyAction([&](int i, int j) {
        float x = rMin+i*m_dr;
        float y = rMin+j*m_dr;

        m_solutionPrevious[i][j] = exp(-(pow(x - x0,2)+pow(y - y0,2))/(2*standardDeviation*standardDeviation));
        m_solution[i][j] = m_solutionPrevious[i][j];

        maxValue = std::max(maxValue,fabs(m_solution[i][j]));
    });

    applyAction([&](int i, int j) {
        m_solutionPrevious[i][j] *= amplitude/maxValue;
        m_solution[i][j] *= amplitude/maxValue;
    });
    calculateWalls();
    calculateMean();
}

void WaveSolver::setGridSize(int gridSize)
{
    m_solution.setGridSize(gridSize);
    m_solutionNext.setGridSize(gridSize);
    m_solutionPrevious.setGridSize(gridSize);
    m_walls.setGridSize(gridSize);
    m_ground.setGridSize(gridSize);
    m_source.setGridSize(gridSize);
    m_gridSize = gridSize;
    m_dr = m_length / (gridSize-1);
}

void WaveSolver::setLength(float length)
{
    m_length = length;
    m_dr = m_length / (gridSize()-1);
}

void WaveSolver::applyAction(std::function<void(int i, int j)> action) {
    for(int i=0; i<gridSize(); i++) {
        for(int j=0; j<gridSize(); j++) {
            action(i,j);
        }
    }
}

void WaveSolver::step(float dt)
{
    float factor = 1.0/(1+0.5*m_dampingFactor*dt);
    float dtdtOverdrdr = dt*dt/(m_dr*m_dr);

    // calculateSource();

    // #pragma omp parallel for private(cx_m, cx_p,cy_m, cy_p, c, ddx, ddy, ddt_rest,i,j) num_threads(1)
    for(unsigned int i=0;i<gridSize();i++) {
        for(unsigned int j=0;j<gridSize();j++) {
            float c = calcC(i,j); // wave speed

            float cx_m = 0.5*(c+calcC(i-1,j)); 	// Calculate the 4 c's we need. We need c_{i \pm 1/2,j} and c_{i,j \pm 1/2}
            float cx_p = 0.5*(c+calcC(i+1,j));
            float cy_m = 0.5*(c+calcC(i,j-1));
            float cy_p = 0.5*(c+calcC(i,j+1));

            float ddx = cx_p*( solution(i,j,1,0)   - m_solution[i][j]) - cx_m*( m_solution[i][j] - solution(i,j,-1,0) );
            float ddy = cy_p*( solution(i,j,0,1)   - m_solution[i][j] ) - cy_m*( m_solution[i][j] - solution(i,j,0,-1) );
            float ddt_rest = -(1-0.5*m_dampingFactor*dt)*m_solutionPrevious[i][j] + 2*m_solution[i][j];

            // Set value to zero if we have a wall.
            m_solutionNext[i][j] = m_walls[i][j] ? 0 : factor*(dtdtOverdrdr*(ddx + ddy) + ddt_rest + m_source[i][j]);
        }
    }

    m_solutionPrevious = m_solution;
    m_solution = m_solutionNext;

    // avg_u = mean(mean(u_));
    calculateWalls();
    calculateMean();
    m_source.zeros();
}