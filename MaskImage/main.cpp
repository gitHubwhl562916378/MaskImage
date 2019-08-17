#include <iostream>
#include <filesystem>
#include <fstream>
#include <execution>
#include <thread>
#include "json/json.h"
#include "opencv.hpp"

#pragma comment(lib, "lib_json.lib")
int main(int argc, char**argv)
{
	try
	{
#if 0 //生成json 配置文件
		Json::Value root;
		root["path-in"] = "images_in";
		root["path-out"] = "images_out";
		
		Json::Value rect;
		rect["x"] = 0;
		rect["y"] = 0;
		rect["width"] = 100;
		rect["height"] = 100;

		root["rects"].append(rect);
		rect["x"] = 50;
		rect["y"] = 50;
		rect["width"] = 200;
		rect["height"] = 200;
		root["rects"].append(rect);
		std::ofstream ofs("config.json" , std::ofstream::out);
		ofs << root.toStyledString();
		ofs.flush();
#endif
		//读取json配置
		std::ifstream file("config.json", std::ifstream::in);

		Json::CharReaderBuilder rbuilder; 
		rbuilder["collectComments"] = false;
		Json::Value config;
		JSONCPP_STRING errs;
		if (!Json::parseFromStream(rbuilder, file, &config, &errs))
		{
			std::cout << "parse json file error" << std::endl;
			return -1;
		}
		file.close();
		//解析参数
		std::string path_in = config["path-in"].asString();
		std::string path_out = config["path-out"].asString();
		std::vector<cv::Rect> rect_vec;
		int r = config["color"]["r"].asInt();
		int g = config["color"]["g"].asInt();
		int b = config["color"]["b"].asInt();
		cv::Scalar mask_color(b, g, r, 0);

		Json::Value rects = config["rects"];
		for (int i = 0; i < rects.size(); i++)
		{
			Json::Value rect = rects[i];
			cv::Rect rt;
			rt.x = rect["x"].asInt();
			rt.y = rect["y"].asInt();
			rt.width = rect["width"].asInt();
			rt.height = rect["height"].asInt();
			rect_vec.push_back(rt);

			std::cout << "rect x:" << rt.x << " y:" << rt.y << " width:" << rt.width << " height:" << rt.height << std::endl;
		}
		std::vector<std::string> img_names;
		for (auto &p : std::filesystem::recursive_directory_iterator(std::filesystem::path(path_in)))
		{
			if (p.is_regular_file())
			{
				img_names.push_back(p.path().string());
			}
		}
		//并行处理贴图
		std::for_each(std::execution::par, img_names.begin(), img_names.end(), [&](const std::string &file_name) {
			std::cout << "thread " << std::this_thread::get_id() << " is handing " << file_name << std::endl;

			cv::Mat src_img = cv::imread(file_name);
			std::for_each(rect_vec.begin(), rect_vec.end(), [&](const cv::Rect &rect) {
				cv::Mat mask(rect.height, rect.width, src_img.type());
				mask.setTo(mask_color);
				mask.copyTo(src_img(rect));

				std::string file = file_name.substr(file_name.find_last_of("\\") + 1);
				cv::imwrite(path_out + "\\" + file, src_img);
			});
		});
		std::this_thread::sleep_for(std::chrono::seconds(30));
	}
	catch (std::exception &e)
	{
		std::cout << "exeception: " << e.what() << std::endl;
	}

	return 0;
}