# RZ/G Shopping Basket Demo Application Source Code

This repository contains the code required to build the application demo. This demo requires [TensorFlow](https://github.com/tensorflow/tensorflow/tree/v2.3.1) and [OpenCV](https://opencv.org/).

## Manual Build Instructions
### RZ/G2
1. Have a suitable cross toolchain by building `bitbake core-image-qt-sdk -c populate_sdk`
with yocto meta layers described in [meta-rzg2](https://github.com/renesas-rz/meta-rzg2)
and [meta-renesas-ai](https://github.com/renesas-rz/meta-renesas-ai) (copy `.conf` files from meta-tensorfow-lite).
2. Install cross toolchain with `sudo sh ./poky-glibc-x86_64-core-image-qt-sdk-aarch64-toolchain-<SDK Version>.sh`.
3. Set up environment variables with `source /<SDK location>/environment-setup-aarch64-poky-linux`.
4. Run `qmake`.
5. Run `make`.
6. Copy `supermarket_demo_app` to the root filesystem.
7. Copy `shoppingBasketDemo.tflite` to `/opt/Shopping_Basket_Demo`.
8. Run the app with `./supermarket_demo_app`.

### Ubuntu
1. Install dependencies
    ```
    sudo apt install cmake qtbase5-dev qtdeclarative5-dev qt5-default qtmultimedia5-dev qtcreator
    ```

2. Install opencv core and opencv videoio, make sure your version has Gstreamer enabled. Otherwise build and install [OpenCV](https://github.com/opencv/opencv.git)
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
    make -j$(nproc)
    sudo make -j$(nproc) install
    ```

3. Build and install [TensorFlow lite v2.3.1](https://github.com/tensorflow/tensorflow/tree/v2.3.1)
    ```
    git clone  https://github.com/tensorflow/tensorflow.git
    cd tensorflow/
    git checkout v2.3.1
    ./tensorflow/lite/tools/make/download_dependencies.sh
    make -j$(nproc) -f ./tensorflow/lite/tools/make/Makefile
    sudo cp ./tensorflow/lite/tools/make/gen/linux_x86_64/lib/libtensorflow-lite.a /usr/local/lib/
    sudo cp -r tensorflow/ /usr/local/include
    sudo cp -r tensorflow/lite/tools/make/downloads/flatbuffers/include/flatbuffers /usr/local/include
    ```

4. Copy [shoppingBasketDemo.tflite](https://github.com/renesas-rz/meta-renesas-ai-demos/blob/master/meta-shopping-basket-demo/recipes-ai/shopping-basket-demo/files/shoppingBasketDemo.tflite) to `/opt/Shopping_Basket_Demo`
    ```
    sudo mkdir /opt/Shopping_Basket_Demo
    cd /opt/Shopping_Basket_Demo
    sudo wget https://github.com/renesas-rz/meta-renesas-ai-demos/raw/master/meta-shopping-basket-demo/recipes-ai/shopping-basket-demo/files/shoppingBasketDemo.tflite
    ```

5. Build demo application
    ```
    cd rzg-shopping-basket-demo
    qmake
    make -j$(nproc)
    sudo cp supermarket_demo_app /opt/Shopping_Basket_Demo
    ```

6. Run the demo with `/opt/Shopping_Basket_Demo/supermarket_demo_app`
