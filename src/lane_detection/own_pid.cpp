#include <iostream>

class PIDController {
public:
    PIDController(double Kp, double Ki, double Kd, double setpoint)
        : Kp(Kp), Ki(Ki), Kd(Kd), setpoint(setpoint), previous_error(0.0), integral(0.0) {}

    double update(double measured_value, double dt) {
        double error = setpoint - measured_value;
        integral += error * dt;
        double derivative = (error - previous_error) / dt;
        double output = (Kp * error) + (Ki * integral) + (Kd * derivative);
        previous_error = error;
        return output;
    }

private:
    double Kp;
    double Ki;
    double Kd;
    double setpoint;
    double previous_error;
    double integral;
};