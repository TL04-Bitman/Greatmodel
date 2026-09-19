
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <ctime>

// 生成带时间戳的截图文件名
static std::string snapshotName() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);          // 线程安全的本地时间
    char buf[64];
    std::strftime(buf, sizeof(buf), "snapshot_%Y%m%d_%H%M%S.jpg", &tm);
    return buf;
}

int main(int argc, char** argv) {
    int camId = (argc >= 2) ? std::atoi(argv[1]) : 0;
    int w = (argc >= 3) ? std::atoi(argv[2]) : 320;   // 默认
    int h = (argc >= 4) ? std::atoi(argv[3]) : 240;   

    //  打开摄像头
    //  后端在虚拟摄像头上
    cv::VideoCapture cap;
    bool ok = cap.open(camId, cv::CAP_V4L2);
    if (!ok) {                        // 不可用时回退到默认后端
        std::cerr << "V4L2 后端打开失败, 尝试默认后端..." << std::endl;
        ok = cap.open(camId);
    }
    if (!ok) {
        std::cerr << "无法打开摄像头: " << camId << std::endl;
        std::cerr << "提示: 可先查看 /dev/video* 确认设备编号" << std::endl;
        return -1;
    }

    // 强制 MJPG 压缩格式 + 指定分辨率
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    cap.set(cv::CAP_PROP_FRAME_WIDTH, w);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, h);

    // 显示实际协商到的像素格式与分辨率
    double fc = cap.get(cv::CAP_PROP_FOURCC);
    char fourccStr[5] = {(char)((int)fc & 0xff), (char)(((int)fc >> 8) & 0xff),
                         (char)(((int)fc >> 16) & 0xff), (char)(((int)fc >> 24) & 0xff), 0};

    std::cout << "摄像头 " << camId << " 已打开, 分辨率="
              << (int)cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
              << (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT)
              << "  像素格式=" << fourccStr << std::endl;
    std::cout << "按键说明: s=保存截图  q/ESC=退出" << std::endl;

    cv::Mat frame;
    double fps = 0.0;
    int64 startTick = cv::getTickCount();
    int frameCount = 0;

    // 循环采集并显示画面
    while (true) {
        cap >> frame;                     // 从摄像头抓取一帧
        if (frame.empty()) {
            std::cerr << "获取画面失败, 摄像头可能被占用" << std::endl;
            break;
        }

        // 计算帧率: 每 30 帧刷新一次
        ++frameCount;
        if (frameCount % 30 == 0) {
            double sec = (cv::getTickCount() - startTick) / cv::getTickFrequency();
            fps = frameCount / sec;
            startTick = cv::getTickCount();
            frameCount = 0;
        }

        // 在画面上叠加按键提示与帧率
        char info[128];
        std::snprintf(info, sizeof(info), "s: save   q/ESC: quit   FPS: %.1f", fps);
        cv::putText(frame, info, cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);

        cv::imshow("Camera " + std::to_string(camId), frame);

        // 处理按键
        int key = cv::waitKey(1);
        if (key == 's' || key == 'S') {
            std::string name = snapshotName();
            if (cv::imwrite(name, frame))
                std::cout << "已保存截图: " << name << std::endl;
            else
                std::cerr << "保存截图失败: " << name << std::endl;
        } else if (key == 'q' || key == 'Q' || key == 27) {  // q 或 ESC 退出
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    std::cout << "程序已退出" << std::endl;
    return 0;
}

