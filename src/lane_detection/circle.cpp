#include <opencv2/opencv.hpp>

class Circle {
    public:
        //cx,cy is center point of the circle 
        cv::Point ClosestIntersection(float cx, float cy, float radius,
                                        cv::Point lineStart, cv::Point lineEnd) {
            cv::Point intersection1;
            cv::Point intersection2;
            int intersections = FindLineCircleIntersections(cx, cy, radius, lineStart, lineEnd, &intersection1, &intersection2);

            if (intersections == 1)
                return intersection1; // one intersection

            if (intersections == 2)
            {
                double dist1 = Distance(intersection1, cv::Point(200,100));
                double dist2 = Distance(intersection2, cv::Point(200,100));

                if (dist1 < dist2)
                    return intersection1;
                else
                    return intersection2;
            }

            return cv::Point(-1,-1); // no intersections at all
        }

    private: 
        double Distance(cv::Point p1, cv::Point p2)
        {
            return std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));
        }

        // Find the points of intersection.
        int FindLineCircleIntersections(float cx, float cy, float radius,
            cv::Point point1, cv::Point point2,               
            cv::Point *intersection1, cv::Point *intersection2) {
            float dx, dy, A, B, C, det, t;

            dx = point2.x - point1.x;
            dy = point2.y - point1.y;

            A = dx * dx + dy * dy;
            B = 2 * (dx * (point1.x - cx) + dy * (point1.y - cy));
            C = (point1.x - cx) * (point1.x - cx) + (point1.y - cy) * (point1.y - cy) - radius * radius;

            det = B * B - 4 * A * C;
            if ((A <= 0.0000001) || (det < 0))
            {
                // No real solutions.
                *intersection1 = cv::Point(-1, -1);
                *intersection2 = cv::Point(-1, -1);
                return 0;
            } else if (det == 0) {
                // One solution.
                t = -B / (2 * A);
                *intersection1 = cv::Point(point1.x + t * dx, point1.y + t * dy);
                *intersection2 = cv::Point(-1, -1);
                return 1;
            } else {
                // Two solutions.
                t = (float)((-B + std::sqrt(det)) / (2 * A));
                *intersection1 = cv::Point(point1.x + t * dx, point1.y + t * dy);
                t = (float)((-B - std::sqrt(det)) / (2 * A));
                *intersection2 = cv::Point(point1.x + t * dx, point1.y + t * dy);
                return 2;
            }
        }
};