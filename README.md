<img src="https://static.wikia.nocookie.net/arnelify/images/c/c8/Arnelify-logo-2024.png/revision/latest?cb=20240701012515" style="width:336px;" alt="Arnelify Logo" />

![Arnelify POD for C++](https://img.shields.io/badge/Arnelify%20POD%20for%20C++-0.5.9-yellow) ![C++](https://img.shields.io/badge/C++-2b-red) ![G++](https://img.shields.io/badge/G++-14.2.0-blue) ![C-Lang](https://img.shields.io/badge/CLang-14.0.6-blue)

## 🚀 About
**Arnelify® POD for C++** - is a BackEnd-framework for creating scalable microservices applications.

## 📋 Minimal Requirements
> Important: It's strongly recommended to use in a container that has been built from the gcc v14.2.0 image.
* CPU: Apple M1 / Intel Core i7 / AMD Ryzen 7
* OS: Debian 11 / MacOS 15 / Windows 10 with <a href="https://learn.microsoft.com/en-us/windows/wsl/install">WSL2</a>.
* RAM: 4 GB

## 📦 Installation
Install to the folder that needs to be created:
```
git clone https://github.com/arnelify/arnelify-server-cpp.git
```
Run Docker:
```
docker compose up -d --build
docker ps
docker exec -it <CONTAINER ID> bash
```
Run setup:
```
make setup
```
## 🎉 Usage
Compile & Run production:
```
make build && pod/server
```
Compile & Run development:
```
make watch
```
## 📚 Code Examples
Configure the C/C++ IntelliSense plugin for VSCode (optional).
```
Clang_format_fallback = Google
```

IncludePath for VSCode (optional):
```
"includePath": [
  "/opt/homebrew/Cellar/jsoncpp/1.9.6/include/json",
  "${workspaceFolder}/src/cpp",
  "${workspaceFolder}/src"
],
```
You can find code examples <a href="https://github.com/arnelify/arnelify-server-cpp/blob/main/src/routes.cpp">here</a>.

## ⚖️ MIT License
This software is licensed under the <a href="https://github.com/arnelify/arnelify-server-cpp/blob/main/LICENSE">MIT License</a>. The original author's name, logo, and the original name of the software must be included in all copies or substantial portions of the software.

## 🛠️ Contributing
Join us to help improve this software, fix bugs or implement new functionality. Active participation will help keep the software up-to-date, reliable, and aligned with the needs of its users.


## ⭐ Release Notes
Version 0.5.9 - Minimalistic dynamic library

We are excited to introduce the Arnelify Server for C++ dynamic library! Please note that this version is raw and still in active development.

Change log:

* Development mode: replaced the g++ with clang++.
* New <a href="https://github.com/arnelify/arnelify-server-cpp">Arnelify Server</a>.
* New <a href="https://github.com/arnelify/arnelify-router-cpp">Arnelify Router</a>
* New <a href="https://github.com/arnelify/arnelify-broker-cpp">Arnelify Broker</a>
* Significant refactoring and optimizations

Please use this version with caution, as it may contain bugs and unfinished features. We are actively working on improving and expanding the framework's capabilities, and we welcome your feedback and suggestions.

## 🔗 Mentioned

* <a href="https://github.com/arnelify/arnelify-pod-cpp">Arnelify POD for C++</a>
* <a href="https://github.com/arnelify/arnelify-pod-python">Arnelify POD for Python</a>
* <a href="https://github.com/arnelify/arnelify-pod-node">Arnelify POD for NodeJS</a>
* <a href="https://github.com/arnelify/arnelify-react-native">Arnelify React Native</a>