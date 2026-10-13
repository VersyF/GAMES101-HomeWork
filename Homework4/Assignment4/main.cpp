#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata) 
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4) 
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
        << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }     
}

// 把曲线点集用 MSAA 反走样画到 window 上（红色通道）
// samples    : 曲线上的采样点（连续浮点坐标）
// window     : 目标图像，必须是 CV_8UC3
// S          : 每像素子采样数（S x S），建议 2~8
// halfWidth  : 线宽的一半，决定覆盖阈值
// colorIndex : 写哪个通道（0=B, 1=G, 2=R）
void antialiasMSAA_boundingBox(const std::vector<cv::Point2f>& samples,
                   cv::Mat& window,
                   int S = 4,
                   float halfWidth = 0.5f,
                   int colorIndex = 2)
{
    if (window.empty() || window.type() != CV_8UC3) {
        return;
    }
    if (samples.empty()) {
        return;
    }

    // ---------- 1. 浮点缓冲区 ----------
    cv::Mat buffer(window.rows, window.cols, CV_32FC1, cv::Scalar(0.0f));

    // ---------- 2. 计算包围盒 ----------
    float minX = samples[0].x, maxX = samples[0].x;
    float minY = samples[0].y, maxY = samples[0].y;
    for (const auto& p : samples) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }

    // margin 覆盖线宽和子采样影响范围
    float margin = halfWidth + 1.0f;
    int x0 = std::max(0, static_cast<int>(std::floor(minX - margin)));
    int y0 = std::max(0, static_cast<int>(std::floor(minY - margin)));
    int x1 = std::min(window.cols - 1, static_cast<int>(std::ceil(maxX + margin)));
    int y1 = std::min(window.rows - 1, static_cast<int>(std::ceil(maxY + margin)));

    if (x1 < x0 || y1 < y0) {
        return;   // 曲线完全在图像外
    }

    // ---------- 3. 包围盒内做 MSAA ----------
    const float invS = 1.0f / S;
    const float halfWidth2 = halfWidth * halfWidth;

    for (int y = y0; y <= y1; ++y) {
        float* bufRow = buffer.ptr<float>(y);
        for (int x = x0; x <= x1; ++x) {
            int hits = 0;

            for (int sy = 0; sy < S; ++sy) {
                for (int sx = 0; sx < S; ++sx) {
                    // 子采样点的连续坐标（取子像素中心）
                    float px = x + (sx + 0.5f) * invS;
                    float py = y + (sy + 0.5f) * invS;

                    // 到曲线点集的最短距离
                    float minDist2 = std::numeric_limits<float>::max();
                    for (const auto& c : samples) {
                        float dx = px - c.x;
                        float dy = py - c.y;
                        float d2 = dx * dx + dy * dy;
                        if (d2 < minDist2) {
                            minDist2 = d2;
                        }
                    }

                    if (minDist2 <= halfWidth2) {
                        ++hits;
                    }
                }
            }

            float coverage = static_cast<float>(hits) / (S * S);
            if (coverage > bufRow[x]) {
                bufRow[x] = coverage;   // 取最大值累积
            }
        }
    }

    // ---------- 4. 统一写入 window ----------
    for (int y = 0; y < window.rows; ++y) {
        const float* bufRow = buffer.ptr<float>(y);
        cv::Vec3b* imgRow = window.ptr<cv::Vec3b>(y);
        for (int x = 0; x < window.cols; ++x) {
            float coverage = bufRow[x];
            if (coverage <= 0.0f) {
                continue;
            }
            uchar value = static_cast<uchar>(std::min(1.0f, coverage) * 255);
            if (value > imgRow[x][colorIndex]) {
                imgRow[x][colorIndex] = value;
            }
        }
    }
}

#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>
#include <limits>

// 用标记缓冲 + MSAA 把曲线画到 window（红色通道）
// samples    : 曲线上的浮点采样点
// window     : 目标图像，必须 CV_8UC3
// S          : 每像素子采样数（S x S）
// halfWidth  : 线宽的一半，决定覆盖阈值
// colorIndex : 写哪个通道（0=B, 1=G, 2=R）
void antialiasMSAA_markBox(const std::vector<cv::Point2f>& samples,
                   cv::Mat& window,
                   int S = 4,
                   float halfWidth = 0.5f,
                   int colorIndex = 2)
{
    if (window.empty() || window.type() != CV_8UC3) return;
    if (samples.empty()) return;

    const int rows = window.rows;
    const int cols = window.cols;

    // ---------- 1. 浮点强度缓冲 + 标记缓冲 ----------
    cv::Mat buffer(rows, cols, CV_32FC1, cv::Scalar(0.0f));
    cv::Mat mask(rows, cols, CV_8UC1, cv::Scalar(0));   // 标记缓冲

    // ---------- 2. 遍历曲线点，标记附近 3x3 格子 ----------
    for (const auto& p : samples) {
        int px = cvRound(p.x);
        int py = cvRound(p.y);

        for (int dy = -1; dy <= 1; ++dy) {
            int y = py + dy;
            if (y < 0 || y >= rows) continue;
            uchar* maskRow = mask.ptr<uchar>(y);

            for (int dx = -1; dx <= 1; ++dx) {
                int x = px + dx;
                if (x < 0 || x >= cols) continue;
                maskRow[x] = 1;              // 标记
            }
        }
    }

    // ---------- 3. 只对标记过的像素做 MSAA ----------
    const float invS = 1.0f / S;
    const float halfWidth2 = halfWidth * halfWidth;

    for (int y = 0; y < rows; ++y) {
        const uchar* maskRow = mask.ptr<uchar>(y);
        float* bufRow = buffer.ptr<float>(y);

        for (int x = 0; x < cols; ++x) {
            if (!maskRow[x]) continue;       // 未标记，跳过

            int hits = 0;
            for (int sy = 0; sy < S; ++sy) {
                for (int sx = 0; sx < S; ++sx) {
                    float px = x + (sx + 0.5f) * invS;
                    float py = y + (sy + 0.5f) * invS;

                    float minDist2 = std::numeric_limits<float>::max();
                    for (const auto& c : samples) {
                        float dx = px - c.x;
                        float dy = py - c.y;
                        float d2 = dx * dx + dy * dy;
                        if (d2 < minDist2) minDist2 = d2;
                    }
                    if (minDist2 <= halfWidth2) ++hits;
                }
            }

            float coverage = static_cast<float>(hits) / (S * S);
            if (coverage > bufRow[x]) bufRow[x] = coverage;
        }
    }

    // ---------- 4. 统一写入 window（只处理有覆盖的像素） ----------
    for (int y = 0; y < rows; ++y) {
        const float* bufRow = buffer.ptr<float>(y);
        cv::Vec3b* imgRow = window.ptr<cv::Vec3b>(y);

        for (int x = 0; x < cols; ++x) {
            float coverage = bufRow[x];
            if (coverage <= 0.0f) continue;

            uchar value = static_cast<uchar>(std::min(1.0f, coverage) * 255);
            if (value > imgRow[x][colorIndex]) {
                imgRow[x][colorIndex] = value;
            }
        }
    }
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window) 
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001) 
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                 3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3;

        window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
    }
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t) 
{
    // TODO: Implement de Casteljau's algorithm

    cv::Point2f A1, A2, A3, A4;
    cv::Point2f B1, B2, B3;
    cv::Point2f C1, C2;
    cv::Point2f D;

    A1 = control_points[0];
    A2 = control_points[1];
    A3 = control_points[2];
    A4 = control_points[3];

    B1 = (1-t)*A1 + t*A2;
    B2 = (1-t)*A2 + t*A3;
    B3 = (1-t)*A3 + t*A4;

    C1 = (1-t)*B1 + t*B2;
    C2 = (1-t)*B2 + t*B3;

    D = (1-t)*C1 + t*C2;

    return cv::Point2f(D.x, D.y);

}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window) 
{
    // TODO: Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's 
    float step = 0.001f;
    std::vector<cv::Point2f> samples;

    for (double i = 0 ; i <= 1 ; i += step){
        cv::Point2f point(0,0);
        point = recursive_bezier(control_points, i);
        samples.push_back(point);
        
        //window.at<cv::Vec3b>(point.y, point.x)[1] = 255;
        
    }
    antialiasMSAA_markBox(samples, window, 4, 0.5f, 1);

    // recursive Bezier algorithm.

}




int main() 
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    int key = -1;
    while (key != 27) 
    {
        for (auto &point : control_points) 
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        if (control_points.size() == 4) 
        {
            //naive_bezier(control_points, window);
            bezier(control_points, window);

            cv::imshow("Bezier Curve", window);
            cv::imwrite("my_bezier_curve.png", window);
            key = cv::waitKey(0);

            return 0;
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

return 0;
}
