# Using Chromium Embedded Framework as the editor GUI of VST on Visual Studio
## Step 1
Prepare the sdk and framework that we need
* vst sdk (I used version 2.4)
* cef(check the official link)
* Microsoft Visual Studio (I used Visual Studio 2013 community to do the whole work)

## Step 2
* Create VS 2013 project by CMake

Make sure you have got the VS installed before using CMake. This step is quite straightforward so I will skip it.

## Step 3
Let's write some code
* Part 1: Create a vst editor class.
Here, I provide the main implementations of them.


(1) Create a derived class of AEffEditor

(2) Open the CEF as another process

(3) Communicate with cef using any IPC technique. Here I use pipe.

* Part 2: Base on message_router example from cef to create a stand-alone application that can render javascript and communicate with C++

(1) Write the browser-side C++ logic to handle request from GUI. If there's a GUI action, then it will send a request to C++, and create a pipe to communicate with vst.

(2) Write a GUI and its corresponding functionality by JavaScript

(3) To let the window created by cef be under the window provided by vst, we need to take care of the "hwnd". I simply cast it to int and cast it back to make it work.

## Step 4
Verify by the mini host provided by the vst sdk. Then we are done.
