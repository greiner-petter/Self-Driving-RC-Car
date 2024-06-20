#include <vector>
#include <unistd.h>
#include <opencv2/opencv.hpp>

const int IMAGE_HEIGHT = 400;
const int IMAGE_WIDTH = 400;

class InvalidPoint : public std::exception {};
class InvalidCircle : public std::exception {};
class UnfittingLaneWidth : public std::exception {};

class Helper {
    public:
        cv::Mat* matrix;

        double calc_dist(cv::Point p1, cv::Point p2) {
            return calc_dist(std::pair(p1.x, p1.y), std::pair(p2.x, p2.y));
        }

        double calc_dist(std::pair<double, double> p1, std::pair<double, double> p2) {
            return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
        }

        int calculate_radius(cv::Mat* matrix) {
            this->matrix = matrix;
            
            const int INITIAL_RADIUS = 50;
            const int FINAL_RADIUS = 250;
            float angle = 0;

            cv::Point* previous_center = nullptr;
            int previous_center_radian = -1;

            std::vector<cv::Point> center_point_list;

            for(int radius = INITIAL_RADIUS; radius < FINAL_RADIUS; radius += 25) {
                std::vector<cv::Point> point_list = get_pointlist_of_radius(radius);

                if(previous_center != nullptr) {
                    previous_center_radian = int(std::atan((400 - previous_center->y) / (previous_center->x - 200)) * 100);
                }
                
                try {
                    cv::Point point = get_street_middle_from_points(point_list, previous_center_radian, radius);

                    center_point_list.push_back(point);
                    previous_center = &point;

                    cv::circle(*matrix, point, 2, (0, 255, 255, 1), 2);
                } catch(const UnfittingLaneWidth& e) {} // Just an unfitting lane width so no need to worry abt an actual error
            }

            cv::Point final_center;
            int final_radius;

            std::tie(final_center, final_radius) = fit_circle_with_fixed_point_ransac(center_point_list, cv::Point(200, 400));

            cv::circle(*matrix, final_center, final_radius, (255,0,0,1), 1);

            for(int radius = INITIAL_RADIUS; radius < FINAL_RADIUS; radius += 25) {
                cv::circle(*matrix, cv::Point(200, 400), radius, (255,255,255,1), 1);
            }

            return final_radius * (final_center.x < 200 ? -1 : 1);
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

        double radius_to_angle(float radius) {
            if(radius > 0) {
                return 3000000.0 * std::pow(radius, -3) + 10;
            } else if(radius < 0) {
                return 2700000.0 * std::pow(radius, -3) - 10;
            } else {
                return 0;
            }
        }

    private:
        void get_two_random_element_indexes (const int size, int &first, int &second) {
            // pick a random element
            first = rand () * size / 2;
            // pick a random element from what's left (there is one fewer to choose from)...
            second = rand () * (size - 1) / 2;
            // ...and adjust second choice to take into account the first choice
            if (second >= first)
            {
                ++second;
            }
        }

        cv::Point check_for_valid_point(int direction, int radius, float looking_pi) {
            std::pair<int, int> pair[2];

            for (double pi = 0; pi < 1; pi += 0.01) {
                pi = looking_pi + pi * direction;

                int x = 200 + round(cos(pi) * radius);
                int y = 400 - round(sin(pi) * radius);

                int x2 = 200 + round(cos(pi+0.01 * direction) * radius);
                int y2 = 400 - round(sin(pi+0.01 * direction) * radius);

                int x3 = 200 + round(cos(pi+0.03 * direction) * radius);
                int y3 = 400 - round(sin(pi+0.03 * direction) * radius);

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

                if ((color2 - color) * direction > 5 && (color3 - color) * direction > 5) {
                    pair[0] = std::pair(x,y);
                }

                if((color - color2) * direction > 5 && pair[0].first != 0 && pair[0].second != 0 && (color - color3) * direction > 5) {
                    pair[1] = std::pair(x,y);

                    double dist = calc_dist(pair[0], pair[1]);

                    if(dist <= 10) {
                        return cv::Point((pair[0].first + pair[1].first) / 2, (pair[0].second + pair[1].second) / 2);
                    }
                }
            }

            throw InvalidPoint();
        }

        cv::Point get_street_middle_from_points(std::vector<cv::Point> point_list, int previous_center, int radius) {
            if (previous_center == -1) {
                previous_center = 1.57;
            }

            int x = 200 + round(std::cos(previous_center) * radius);

            std::vector<cv::Point> right_pointlist, left_pointlist;

            for (const auto& point : point_list) {
                if (point.x - x > 0) {
                    right_pointlist.push_back(point);
                } else {
                    left_pointlist.push_back(point);
                }
            }

            std::sort(right_pointlist.begin(), right_pointlist.end(), [&](const cv::Point& a, const cv::Point& b) {
                return a.x - x < b.x - x;
            });

            std::sort(left_pointlist.begin(), left_pointlist.end(), [&](const cv::Point& a, const cv::Point& b) {
                return abs(a.x - x) < abs(b.x - x);
            });

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
                return cv::Point((right_pointlist[0].x + left_pointlist[0].x) / 4, (right_pointlist[0].y + left_pointlist[0].y) / 4);
            } else {
                throw UnfittingLaneWidth();
            }
        }

        std::vector<cv::Point> get_pointlist_of_radius(int radius) {
            std::vector<cv::Point> point_list;

            for(float pi = 0; pi < 3.14; pi += 0.01) {
                try {
                    cv::Point point = check_for_valid_point(1, radius, pi);

                    if(std::find(point_list.begin(), point_list.end(), point) == point_list.end()) {
                        point_list.push_back(point);
                    }
                } catch(const InvalidPoint& e) {} // Just an invalid point so no need to worry abt an actual error
            }

            return point_list;
        }

        std::pair<cv::Point, int> calculate_circle_center_radius(cv::Point p1, cv::Point p2, cv::Point fixed_point) {
            float ma;
            float mb;

            if(p2.x != p1.x && p2.y != p1.y) {
                ma = (p2.y - p1.y) / (p2.x - p1.x);
            } else {
                ma = std::numeric_limits<float>::infinity();
            }

            if(fixed_point.x != p2.x && fixed_point.y != p2.y) {
                mb = (fixed_point.y - p2.y) / (fixed_point.x - p2.x);
            } else {
                mb = std::numeric_limits<float>::infinity();
            }

            if(ma == mb) {
                throw InvalidCircle();
            }

            int cx = (ma * mb * (p1.y - fixed_point.y) + mb * (p1.x + p2.x) - ma * (p2.x + fixed_point.x)) / (2 * (mb - ma));
            int cy;

            if(ma != std::numeric_limits<float>::infinity()) {
                cy = -1 * (cx - (p1.x + p2.x) / 2) / ma + (p1.y + p2.y) / 2;
            } else {
                cy = (cx - (p2.x + fixed_point.x) / 2) / mb + (p2.y + fixed_point.y) / 2;
            }

            int radius = std::sqrt(std::pow(cx - p1.x, 2) + std::pow(cy - p1.y, 2));

            return std::pair(cv::Point(cx, cy), radius);
        }

        std::pair<cv::Point, int> fit_circle_with_fixed_point_ransac(std::vector<cv::Point> points, cv::Point fixed_point) {
            const int MAX_ITERATIONS = 1000;
            const float DISTANCE_THRESHOLD = 1;
            const float MIN_INLIERS_RATIO = 0.5;

            std::pair<cv::Point, int>* best_circle = nullptr;
            std::vector<cv::Point> best_inliners;
            int num_points = points.size();

            for (int _ = 0; _ < MAX_ITERATIONS; _++) {
                int first_index;
                int second_index;

                get_two_random_element_indexes(points.size(), first_index, second_index);

                cv::Point sample[2] = {
                    points[first_index],
                    points[second_index]
                };

                std::pair<cv::Point, int> circle_center_radius;

                try {
                    circle_center_radius = calculate_circle_center_radius(sample[0], sample[1], fixed_point);
                } catch(const InvalidCircle& e) { // Just an invalid circle so no need to worry abt an actual error
                    continue;
                }
                
                cv::Point center = circle_center_radius.first;
                int radius = circle_center_radius.second;

                std::vector<cv::Point> inliers;

                for(cv::Point p : points) {
                    if(std::abs(std::sqrt(std::pow(p.x - center.x, 2) + std::pow(p.y - center.y, 2)) - radius) < DISTANCE_THRESHOLD) {
                        inliers.push_back(p);
                    }
                }

                if(inliers.size() > MIN_INLIERS_RATIO * num_points && inliers.size() > best_inliners.size()) {
                    best_circle = &circle_center_radius;
                    best_inliners = inliers;
                }
            }

            if(best_circle == nullptr) {
                throw InvalidCircle();
            }

            int h, k ,r;
            
            std::tie(h, k ,r) = fit_circle_with_fixed_point(points, fixed_point);

            return std::pair(cv::Point(h, k), r);
        }

        std::tuple<int, int, int> fit_circle_with_fixed_point(std::vector<cv::Point> points, cv::Point fixed_point) {
            int x_fixed, y_fixed; 
            x_fixed = fixed_point.x;
            y_fixed = fixed_point.y;

            std::vector<int> x_shifted, y_shifted;

            for(cv::Point p : points) {
                x_shifted.push_back(p.x - x_fixed);
                y_shifted.push_back(p.y - y_fixed);
            }

            cv::Mat A(points.size(), 2, CV_32F);
            cv::Mat b(points.size(), 1, CV_32F);

            for (size_t i = 0; i < points.size(); ++i) {
                A.at<float>(i, 0) = 2 * x_shifted[i];
                A.at<float>(i, 1) = 2 * y_shifted[i];
                b.at<float>(i, 0) = std::pow(x_shifted[i], 2) + std::pow(y_shifted[i], 2);
            }

            cv::Mat h_k_shifted;
            cv::solve(A, b, h_k_shifted, cv::DECOMP_NORMAL);

            float h_shifted = h_k_shifted.at<float>(0, 0);
            float k_shifted = h_k_shifted.at<float>(1, 0);

            // Step 4: Convert the coordinates back to the original coordinate system
            float h = h_shifted + x_fixed;
            float k = k_shifted + y_fixed;

            float r = std::sqrt(std::pow(h - x_fixed, 2) + std::pow(k - y_fixed, 2));

            return std::make_tuple(static_cast<int>(h), static_cast<int>(k), static_cast<int>(r));
        }
};