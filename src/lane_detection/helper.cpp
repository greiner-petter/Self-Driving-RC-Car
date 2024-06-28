#include <vector>
#include <unistd.h>
#include <opencv2/opencv.hpp>

const int IMAGE_HEIGHT = 400;
const int IMAGE_WIDTH = 400;
const int COLOR_DIFFERENCE = 5;

class Helper {
    public:
        cv::Mat* matrix;
        cv::Mat* drawMatrix;

        double calc_dist(cv::Point p1, cv::Point p2) {
            return calc_dist(std::pair(p1.x, p1.y), std::pair(p2.x, p2.y));
        }

        double calc_dist(std::pair<double, double> p1, std::pair<double, double> p2) {
            return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
        }

        int calculate_radius(cv::Mat* matrix, cv::Mat* drawMatrix) {
            this->matrix = matrix;
            this->drawMatrix = drawMatrix;

            if(std::getenv("CAR_ENV") != NULL) {
                cv::cvtColor(*this->drawMatrix, *this->drawMatrix, cv::COLOR_GRAY2RGB);
            }
            
            const int INITIAL_RADIUS = 50;
            const int FINAL_RADIUS = 200;
            const int ZIRKLE_DIFF = 25;

            cv::Point* previous_center = nullptr;
            float previous_center_radian = -1;

            std::vector<cv::Point> center_point_list;

            for(int radius = INITIAL_RADIUS; radius < FINAL_RADIUS; radius += ZIRKLE_DIFF) {
                std::vector<cv::Point> point_list = get_pointlist_of_radius(radius);

                if(previous_center != nullptr) {
                    float dy = previous_center->y - 400;
                    float dx = previous_center->x - 200;
                    previous_center_radian = std::atan2(dx, dy);
                }
                
                cv::Point point = get_street_middle_from_points(point_list, previous_center_radian, radius);

                if(point.x != -1) {
                    center_point_list.push_back(point);
                    previous_center = &point;

                    if(std::getenv("CAR_ENV") != NULL) {
                        cv::circle(*drawMatrix, point, 2, cv::Scalar(0, 255, 255, 1), 2); //gelb
                    }
                }
            }

            if(std::getenv("CAR_ENV") != NULL) {
                for(int radius = INITIAL_RADIUS; radius < FINAL_RADIUS; radius += ZIRKLE_DIFF) {
                    cv::circle(*drawMatrix, cv::Point(200, 400), radius, cv::Scalar(255,255,255,1), 1); //weiße radien kreise
                }
            }

            #ifdef DEBUG
                cv::imshow("Lane Detection", *drawMatrix);
                char key = cv::waitKey(30);
                if (key == 'q')
                {
                    cv::destroyAllWindows();
                    return 0;
                }
            #endif

            cv::Point final_center;
            int final_radius;

            std::tie(final_center, final_radius) = loop_through_circles(center_point_list);

            if(std::getenv("CAR_ENV") != NULL) {
                cv::circle(*this->drawMatrix, final_center, abs(final_radius), cv::Scalar(200, 110, 50, 255), 5); //end radius kreis
            }

            return final_radius;
        }

        double radius_to_angle(float radius) {
            if(radius > 0) {
                return 3000000.0 * std::pow(radius, -3) + 10;
            } else if(radius < 0) {
                return 2700000.0 * std::pow(radius, -3) - 10;
            } else {
                return 0;
            }
        }

        double map(float X) {
            //3125 Y = (X-A)/(B-A) * (D-C) + C
            if(X < 0) {
                return -map(-X);
            }

            return (X-0)/(3.14-0) * (0-180) + 180;
        }

    private:
        cv::Point check_for_valid_point(int direction, int radius, float looking_pi) {
            std::pair<int, int> pair[2];

            for (double pi = 0; pi < 0.5; pi += 0.01) {
                double offset = looking_pi + pi * direction;

                int x = 200 + round(cos(offset) * radius);
                int y = 400 - round(sin(offset) * radius);

                int x2 = 200 + round(cos(offset+0.01 * direction) * radius);
                int y2 = 400 - round(sin(offset+0.01 * direction) * radius);

                int x3 = 200 + round(cos(offset+0.03 * direction) * radius);
                int y3 = 400 - round(sin(offset+0.03 * direction) * radius);

                if (x >= IMAGE_WIDTH || x < 0 || y >= IMAGE_HEIGHT || y < 0) {
                    continue;
                }

                if (x2 >= IMAGE_WIDTH || x2 < 0 || y2 >= IMAGE_HEIGHT || y2 < 0) {
                    continue;
                }

                if (x3 >= IMAGE_WIDTH || x3 < 0 || y3 >= IMAGE_HEIGHT || y3 < 0) {
                    continue;
                }

                int color = matrix->at<uint8_t>(y, x);
                int color2 = matrix->at<uint8_t>(y2, x2);
                int color3 = matrix->at<uint8_t>(y3, x3);

                if (color > 235) {
                    continue;
                }

                if ((color2 - color) * direction > COLOR_DIFFERENCE && (color3 - color) * direction > COLOR_DIFFERENCE) {
                    pair[0] = std::pair(x,y);
                }

                if((color - color2) * direction > COLOR_DIFFERENCE && pair[0].first != 0 && pair[0].second != 0 && (color - color3) * direction > COLOR_DIFFERENCE) {
                    pair[1] = std::pair(x,y);

                    double dist = calc_dist(pair[0], pair[1]);

                    if(dist <= 10.0f) {
                        return cv::Point((pair[0].first + pair[1].first) / 2, (pair[0].second + pair[1].second) / 2);
                    }
                }
            }

            return cv::Point(-1,-1);
        }

        cv::Point get_street_middle_from_points(std::vector<cv::Point> point_list, float previous_center, int radius) {
            if (previous_center == -1) {
                previous_center = 3.14;
            }
                
            int x = 200 + round(std::sin(previous_center) * radius);
            int y = 400 + round(std::cos(previous_center) * radius);

            if(std::getenv("CAR_ENV") != NULL) {
                cv::circle(*this->drawMatrix, cv::Point(int(x), int(y)), 10, cv::Scalar(255,255,255,0), 2); // white


                for(cv::Point point : point_list) {
                    cv::circle(*this->drawMatrix, point, 20, cv::Scalar(0,255,0,0), 1); //green
                }
            }

            std::vector<cv::Point> right_pointlist, left_pointlist;

            for (const auto& point : point_list) {
                if (point.x - x > 0) {
                    right_pointlist.push_back(point);
                } else {
                    left_pointlist.push_back(point);
                }
            }

            if(std::getenv("CAR_ENV") != NULL) {
                for(cv::Point left : left_pointlist) {
                    cv::circle(*this->drawMatrix, left, 20, cv::Scalar(0,0,255,0), 1); //red
                }
            }

            std::sort(right_pointlist.begin(), right_pointlist.end(), [&](const cv::Point& a, const cv::Point& b) {
                return a.x - x < b.x - x;
            });

            std::sort(left_pointlist.begin(), left_pointlist.end(), [&](const cv::Point& a, const cv::Point& b) {
                return abs(a.x - x) < abs(b.x - x);
            });

            if(right_pointlist.empty() && left_pointlist.empty()) {
                return cv::Point(-1, -1);
            }

            if(right_pointlist.empty()) {
                return cv::Point(left_pointlist[0].x - 25, left_pointlist[0].y);
            }

            if(left_pointlist.empty()) {
                return cv::Point(right_pointlist[0].x - 25, right_pointlist[0].y);
            }

            double dist = calc_dist(right_pointlist[0], left_pointlist[0]);

            if(dist < 70) {
                return cv::Point((right_pointlist[0].x + left_pointlist[0].x) / 2, (right_pointlist[0].y + left_pointlist[0].y) / 2);
            } else if(dist >= 70 && dist <= 130) {
                return cv::Point((3*right_pointlist[0].x + left_pointlist[0].x) / 4, (3*right_pointlist[0].y + left_pointlist[0].y) / 4);
            } else {
                return cv::Point(-1, -1);
            }
        }

        std::vector<cv::Point> get_pointlist_of_radius(int radius) {
            std::vector<cv::Point> point_list;

            for(float pi = 0; pi < 3.14; pi += 0.1f) {
                cv::Point point = check_for_valid_point(1, radius, pi);

                if(point.x != -1 && std::find(point_list.begin(), point_list.end(), point) == point_list.end()) {
                    point_list.push_back(point);
                }
            }

            return point_list;
        }

        std::tuple<cv::Point, int> loop_through_circles(std::vector<cv::Point> points) {
            int best_radius = 100000000;
            cv::Point best_center = cv::Point(0,0);
            double best_dist = 100000000;

            for(float ra = -5; ra < 5; ra+= 0.1) {
                if (std::abs(ra) < 1.1) {
                    continue;
                }

                int r = int(std::pow(ra, 5));
                cv::Point center = cv::Point(200+r, 450);

                double dist = 0;

                for (cv::Point point : points) {
                    dist += abs(calc_dist(center, point) - abs(r));
                }

                if (dist < best_dist) {
                    best_dist = dist;
                    best_radius = r;
                    best_center = center;
                }
            }

            return std::make_tuple(best_center, best_radius);
        }
};