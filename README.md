# OM3D
Made by Melvin Gidel and Antoine Aubin

Training project about 3D engine optimisation. 
It features : 
- OIT using linked list 
- Simple tiles deferred rendering

## Order Independant Transparency using Linked List
Based on this presentation made by Christoph Kubisch : [Order Independent Transparency Opengl](https://on-demand.gputechconf.com/gtc/2014/presentations/S4385-order-independent-transparency-opengl.pdf)

https://user-images.githubusercontent.com/47392735/213946728-e670a655-2e84-4005-9780-788856efffee.mp4



### How to build
Requirements: cmake 3.20 minimum, C++17, and OpenGL 4.5.
```bash
# At the project root
mkdir -p TP/build/debug
cd TP/build/debug
cmake ../..
make
```

_This project is part of an EPITA course made by Alexandre Lamure and Gregoire Angerrand._
