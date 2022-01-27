# Get rid of any previous virtual frame buffer
pkill -9 Xvfb
rm /var/tmp/Xvfb_screen_0

# Create a virtual screen :99.0 with given dimensions & color depth
# mapping output to /var/tmp/Xvfb_screen_0
Xvfb :99 -screen 0 1920x1080x16 -fbdir /var/tmp &

# Run the nbody simulation on this screen
DISPLAY=:99.0 ./nbodygl &

# To take a screenshot instead of a video (doesn't always work):
# sleep 2
# DISPLAY=:99 xwd -root -silent | convert xwd:- png:/tmp/screenshot.png

# Use the x11grab device to write to video file
ffmpeg -video_size 1920x1080 -framerate 25 -f x11grab -i :99.0+0,0 output.mp4


# This script tends to produce slow stuttery output, further investigation required.
# For now, I do something like:
# ffmpeg -i output.mp4 -map 0:v -c:v copy -bsf:v h264_mp4toannexb raw.h264
# ffmpeg -fflags +genpts -r 240 -i raw.h264 -c:v copy output_240.mp4
