#include <iostream>
class own_pid {
private:
    // PID coefficients
    float kp;
    float ki;
    float kd;

    // Previous error and integral of errors
    float prevError;
    float integral;

    // Time step
    float dt;

public:
    // Constructor to initialize the PID coefficients and time step
    own_pid(float kp, float ki, float kd, float dt)
        : kp(kp), ki(ki), kd(kd), dt(dt), prevError(0.0), integral(0.0) {}

    // Method to compute the control variable based on the setpoint and the measured value
    float compute(float setpoint, float measuredValue) {
        // Calculate the error
        float error = setpoint - measuredValue;

        // Calculate the integral of the error
        integral += error * dt;

        // Calculate the derivative of the error
        float derivative = (error - prevError) / dt;

        // Calculate the control variable
        float output = kp * error + ki * integral + kd * derivative;

        // Update the previous error
        prevError = error;

        return output;
    }

    // Setters for PID coefficients
    void setKp(float kp) { this->kp = kp; }
    void setKi(float ki) { this->ki = ki; }
    void setKd(float kd) { this->kd = kd; }
    void setDt(float dt) { this->dt = dt; }
};

int main() {
    // PID coefficients
    float kp = 1.0;
    float ki = 0.1;
    float kd = 0.01;

    // Time step
    float dt = 0.1;

    // Create a PID controller object
    own_pid pid(kp, ki, kd, dt);

    // Setpoint and measured value
    float setpoint = 10.0;
    float measuredValue = 0.0;

    // Simulate the PID controller
    for (int i = 0; i < 100; ++i) {
        // Compute the control output
        float control = pid.compute(setpoint, measuredValue);

        // Update the measured value (for simulation purposes)
        measuredValue += control * dt;

        // Print the results
        std::cout << "Time: " << i * dt << " s, Control: " << control << ", Measured Value: " << measuredValue << std::endl;
    }

    return 0;
}