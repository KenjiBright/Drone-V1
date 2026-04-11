#ifndef KALMAN_1D_H
#define KALMAN_1D_H

struct Kalman1D {
    float state = 0.0f;
    float uncertainty = 4.0f; 

    float update(float rate, float measurement, float dt) {
        state = state + dt * rate;
        uncertainty = uncertainty + (dt * dt * 1.0f * 1.0f);
        
        float gain = uncertainty * 1.0f / (1.0f * uncertainty + 3.0f * 3.0f);
        state = state + gain * (measurement - state);
        uncertainty = (1.0f - gain) * uncertainty;
        
        return state;
    }
};
#endif