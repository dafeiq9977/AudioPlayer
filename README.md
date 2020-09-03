# AudioPlayer
音频播放器

    实现对视频文件中的音频帧提取，并用OpenAL播放。
## 编译方法
将文件下载到指定的工程源文件夹下，建立include文件夹，并向include文件夹下导入FFmpeg和OpenAL头文件；
在工程源文件夹下建立build文件夹作为cmake生成的工程目录；
将FFmpeg动态库文件拷贝到build文件夹下；
使用cmake生成工程；
打开集成开发环境，编译县项目。
## 使用方法
进入可执行文件夹，将FFmpeg动态库文件和test.mp4测试视频文件全部拷贝过来；
在此文件夹下打开cmd窗口，输入可执行文件名称，回车运行。

