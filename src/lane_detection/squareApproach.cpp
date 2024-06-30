#include <opencv2/opencv.hpp>
#include <vector>
#include <unistd.h>

class SquareApproach {
    public:
        float calc_angle(cv::Point center, int radius) {
            bool first_intersection = false;

            float pi = 0;
            float eps = 0.001;

            if(center.x > 200) {
                pi = 2 * 3.14;
                eps = -0.001;
            }

            int last_x = -1;
            int last_y = -1;

            for(; is_pi(center.x > 200, pi); pi+=eps) {
                int x = center.x + std::cos(pi) * radius;
                int y = -50 + std::sin(pi) * radius;

                if(last_x == x && last_y == y) {
                    continue;
                }

                if(is_point_in_bild(x, y)) {
                    continue;
                }

                if(first_intersection && calc_dist(cv::Point(x,y), cv::Point(last_x, last_y)) < 5) {
                    continue;
                }

                if(x == 0) { // links
                    if(!first_intersection) {
                        first_intersection = true;
                        last_x = x;
                        last_y = y;
                        continue;
                    }

                    return -((400 - y) / 400.0 * 50 + 10); // -10 to -60
                } else if(x == 400) { // rechts
                    if(!first_intersection) {
                        first_intersection = true;
                        last_x = x;
                        last_y = y;
                        continue;
                    }

                    return ((400 - y) / 400.0) * 50 + 10; // 10 to 60
                } else if(y == 0 && x >= 200) { // rechts unten
                    if(!first_intersection) {
                        first_intersection = true;
                        last_x = x;
                        last_y = y;
                        continue;
                    }

                    return (400 - x) / 200.0 * 40 + 60; // 60 to 100
                } else if(y == 0 && x <= 200) { // links unten
                    if(!first_intersection) {
                        first_intersection = true;
                        last_x = x;
                        last_y = y;
                        continue;
                    }

                    return -(x / 200.0 * 40 + 60); // -60 to -80
                } else if(y == 400) { // oben
                    if(!first_intersection) {
                        first_intersection = true;
                        last_x = x;
                        last_y = y;
                        continue;
                    }

                    return (x - 200) / 200.0 * 10; // -10 to 10
                }
            }

            return 0;
        }

    private:
        bool is_point_in_bild(int x, int y) {
            return x > 400 || x < 0 || y > 400 || y < 0;
        }

        bool is_pi(bool left, float pi) {
            return left ? pi > 0 : pi < 2*3.14;
        }

        double calc_dist(cv::Point p1, cv::Point p2) {
            return calc_dist(std::pair(p1.x, p1.y), std::pair(p2.x, p2.y));
        }

        double calc_dist(std::pair<double, double> p1, std::pair<double, double> p2) {
            return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
        }
};