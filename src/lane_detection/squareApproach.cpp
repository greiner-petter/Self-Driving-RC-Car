#include <opencv2/opencv.hpp>

class SquareApproach {
    public:
        float calc_angle(cv::Point center, int radius) {
            bool first_intersection = false;

            for(float pi = 0; pi < 3.14; pi+=0.01) {
                int x = center.x + std::cos(pi) * radius;
                int y = center.y + std::sin(pi) * radius;

                if(is_point_in_bild(x, y)) {
                    continue;
                }

                if(x == 0) { // links
                    if(!first_intersection) {
                        first_intersection = true;
                        continue;
                    }

                    return -((400 - y) / 400 * 30 + 10); // -10 to -40
                } else if(x == 400) { // rechts
                    if(!first_intersection) {
                        first_intersection = true;
                        continue;
                    }

                    return (400 - y) / 400 * 30 + 10; // 10 to 40
                } else if(y == 0 && x > 200) { // rechts unten
                    if(!first_intersection) {
                        first_intersection = true;
                        continue;
                    }

                    return (400 - x) / 200 * 25 + 40; // 40 to 65
                } else if(y == 0 && x < 200) { // links unten
                    if(!first_intersection) {
                        first_intersection = true;
                        continue;
                    }

                    return -(x / 200 * 25 + 40); // -40 to -65
                } else if(y == 400) { // oben
                    if(!first_intersection) {
                        first_intersection = true;
                        continue;
                    }

                    return (x - 200) / 200 * 10; // -10 to 10
                }
            }

            return 0;
        }

    private:
        bool is_point_in_bild(int x, int y) {
            return x > 400 || x < 0 || y > 400 || y < 0;
        }
};