# RZ/G Shopping Basket Demo Application Source Code

This repository contains the code required to build the application demo. This demo requires [TensorFlow](https://github.com/tensorflow/tensorflow/tree/r1.10) and [OpenCV](https://opencv.org/).

## Manual Build Instructions
### RZ/G2
1. Have a suitable cross toolchain by building `bitbake core-image-qt-sdk -c populate_sdk`
with yocto meta layers described in [meta-rzg2](https://github.com/renesas-rz/meta-rzg2)
and [meta-renesas-ai](https://github.com/renesas-rz/meta-renesas-ai) (copy `.conf` files from meta-tensorfow-lite).
2. Install cross toolchain with `sudo sh ./poky-glibc-x86_64-core-image-qt-sdk-aarch64-toolchain-2.4.3.sh`.
3. Set up environment variables with `source /<SDK location>/environment-setup-aarch64-poky-linux`.
4. Run `qmake`.
5. Run `make`.
6. Copy `supermarket_demo_app` to the root filesystem.
7. Copy `shoppingBasketDemo.tflite` to `/opt/Shopping_Basket_Demo`.
8. Run the app with `./supermarket_demo_app`.

### Ubuntu
1. Install opencv core and opencv videoio, make sure your version has Gstreamer enabled. otherwise build and install [OpenCV](https://github.com/opencv/opencv.git).
    ```
    git clone  https://github.com/opencv/opencv.git
    cd opencv/
    mkdir build/
    cd build/
    cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D WITH_V4L=ON \
    -D WITH_QT=ON \
    -D WITH_GSTREAMER=ON ..
    make
    make install
    ```

2. Build and install [TensorFlow lite r1.10](https://github.com/tensorflow/tensorflow/tree/r1.10).
    ```
    git clone  https://github.com/tensorflow/tensorflow.git
    cd tensorflow/
    git checkout r1.10
    ./tensorflow/contrib/lite/download_dependencies.sh
    make -f ./tensorflow/contrib/lite/Makefile
    sudo cp ./tensorflow/contrib/lite/gen/lib/libtensorflow-lite.a /usr/local/lib/
    sudo cp -r tensorflow/ /usr/local/include
    sudo cp -r tensorflow/contrib/lite/downloads/flatbuffers/include/flatbuffers/ /usr/local/include
    ```
3. Copy `shoppingBasketDemo.tflite` to `/opt/Shopping_Basket_Demo`.
4. Run `qmake`
5. Run `make`
6. Run the app with `./supermarket_demo_app`
