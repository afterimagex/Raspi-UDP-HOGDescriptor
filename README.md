# Raspi-UDP-HOGDescriptor<br>
Raspi-UDP-HOGDescriptor<br>
PC-Client and Raspi-server<br>

##树莓派行人检测<br>
服务端是树莓派3,客户端是PC端<br>

##Usage:<br>
###PC-Client:<br>
build by VS2013 depend Opencv2.4.9<br>
PC端使用VS2013构建编译项目，需要2.4.9运行库<br>

###Raspi-Server:<br>
g++ cam_server.cpp -o cam_server \`pkg-config --cflags --libs opencv\`<br>
./cam_server
