ADD_EXECUTABLE( ${PROJECT_NAME}
	"AudioPlayer.cpp"
)
TARGET_LINK_LIBRARIES( ${PROJECT_NAME}
	"avcodec.lib"
	"avformat.lib"
	"avutil.lib" 
	"avdevice.lib"
	"avfilter.lib"
	"postproc.lib"
	"swresample.lib"
	"swscale.lib"
	"SDL2.lib"
	"OpenAL32.lib"
)